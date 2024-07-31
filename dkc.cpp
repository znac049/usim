//
//
//	dkc.cpp
//      Emulate a CopactFlash device running in True IDE mode
//
//	(C) Bob Green, 2024
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>

#include "dkc.h"
#include "bits.h"

dkc::dkc()
{
    reset();

	openDisk("disk1.img", 0);
    openDisk("disk2.img", 1);
}

dkc::~dkc()
{
}

bool dkc::openDisk(const char *diskName, int diskNum)
{
	struct stat inf;
	int res = stat(diskName, &inf);

	if (res == 0) {
		FILE *fd = fopen(diskName, "a+");
		if (fd != NULL) {
			long blocks;

			fseek(fd, 0L, SEEK_END);
			blocks = ftell(fd) / BLOCK_SIZE;

			printf("CF Disk%d - disk '%s' opened ok - number of blocks: %ld\r\n", diskNum, diskName, blocks);

			disks[diskNum] = fd;
			numBlocks[diskNum] = blocks;
            block_num = 0;

            checkForDriveChange(-1);

			return true;
		}
	}

    checkForDriveChange(-1);

	return false;
}

Byte dkc::read(Word offset)
{
    Byte val = 0;

	switch (offset) {
		case CF_Data:
            {
                if (readIndex < BLOCK_SIZE) {
                    val = blockBuffer[readIndex++];
                    if (readIndex >= BLOCK_SIZE) {
                        clearStatusBit(SR_DRQ);
                    }
                }
                else {
                    val = 0xff;
                    setStatusBit(SR_ERR);
                    clearStatusBit(SR_DRQ);
                }
        	    // printf("CF: Read CF Data reg(%d) -> %02x\r\n", offset, val);
            }
            break;
            
		case CF_Error:
            val = errorReg;
        	printf("CF: Read CF Error reg(%d) -> %02x\r\n", offset, val);
            break;

		case CF_SecCnt:
            val = sectorCountReg;
          	printf("CF: Read CF SecCnt reg(%d) -> %02x\r\n", offset, val);
             break;
            
		case CF_LSN0:
            val = block_num & 0xff;
          	// printf("CF: Read CF LSN0 reg(%d) -> %02x\r\n", offset, val);
            break;
            
		case CF_LSN1:
            val = (block_num  >> 8) & 0xff;
          	// printf("CF: Read CF LSN1 reg(%d) -> %02x\r\n", offset, val);
            break;
            
		case CF_LSN2:
            val = (block_num >> 16) & 0xff;
          	// printf("CF: Read CF LSN2 reg(%d) -> %02x\r\n", offset, val);
            break;
            
		case CF_LSN3:
            val = (block_num >>24) & 0xff;
          	// printf("CF: Read CF LSN3 reg(%d) -> %02x\r\n", offset, val);
            break;
            
		case CF_Status:
            val = statusReg;
          	// printf("CF: Read CF Status reg(%d) -> %02x\r\n", offset, val);
            break;
            
		default:
            val = 0xff;
        	printf("CF: Read @%d -> %02x\r\n", offset, val);
            break;
	}

    return val;
}

void dkc::write(Word offset, Byte val)
{
	switch (offset) {
		case CF_Data:
            printf("CF: Write %02x to CF Data reg(%d)\r\n", val, offset);
			break;

		case CF_Features:
            printf("CF: Write %02x to CF Features reg(%d)\r\n", val, offset);
            featureReg = val;
			break;

		case CF_SecCnt:
            printf("CF: Write %02x to CF SecCnt reg(%d)\r\n", val, offset);
            sectorCountReg = val;
			break;

		case CF_LSN0:
            // printf("CF: Write %02x to CF LSN0 reg(%d)\r\n", val, offset);
            block_num = (block_num & 0xffffff00) | val;
			break;

		case CF_LSN1:
            // printf("CF: Write %02x to CF LSN1 reg(%d)\r\n", val, offset);
            block_num = (block_num & 0xffff00ff) | val<<8;
			break;

		case CF_LSN2:
            // printf("CF: Write %02x to CF LSN2 reg(%d)\r\n", val, offset);
            block_num = (block_num & 0xff00ffff) | val<<16;
			break;

		case CF_LSN3:
            {
                int drive = getDriveNum();

                // printf("CF: Write %02x to CF LSN3 reg(%d)\r\n", val, offset);
                block_num = (block_num & 0x00ffffff) | val<<24;
                checkForDriveChange(drive);
            }
			break;

		case CF_Command:
            // printf("CF: Write %02x to CF Command reg(%d)\r\n", val, offset);
            cfCommand(val);
			break;

        default:
            printf("CF: Write %02x to %02x\r\n", val, offset);
            break;
	}
}

void dkc::reset(void)
{
  	for (int i=0; i<MAX_DISKS; i++) {
  		disks[i] = NULL;
  	}

    block_num = 0;
    errorReg = 0;
    sectorCountReg = 1;
    statusReg = SR_RDY;

  	fprintf(stderr, "CF: Virtual disk subsystem reset.\r\n");
}

void dkc::cfCommand(Byte cmd)
{
    // I can't find it mentioned in the spec anywhere
    // But I'm assuming that any error conditions get reset
    // whenever a new command is executed.

    setStatusBit(SR_BSY);

    switch (cmd) {
        case CMD_DIAG:
            printf("CF: Execute 'DIAG' command (%02x)\r\n", cmd);
            errorReg = 0x01;    // Code for "No error detected"
            clearStatusBit(SR_BSY);
            break;

        case CMD_SET_FEATURES:
            printf("CF: Execute 'Set Features' command (%02x)\r\n", cmd);
            setFeatures();
            break;

        case CMD_READ_SECTORS:
        case CMD_READ_SECTORS_ALT:
            {
                int block = block_num & 0x0fffffff;
                int drive = (block_num >> 28) & 0x01;
                int count = sectorCountReg;
                FILE *fd = disks[drive];

                if (count == 0) {
                    count = 256;
                }

                readIndex = 0;      // Reset the read buffer counter
    
                // printf("CF: Execute 'Read Sectors' command (%02x)\r\n", cmd);
                // printf("CF: Reading %d block%s from disk %d - block %d\r\n", 
                //         count,
                //         (count==1)?"":"s",
                //         drive, 
                //         block_num & 0x0fffffff);

                if (fd == NULL) {
                    // printf("CF: disk %d not present\r\n", drive);
                    setStatusBit(SR_ERR);
                    clearStatusBit(SR_BSY);
                }
                else {
                    if (count != 1) {
                        // printf("CF: Multi-block reads not supported\r\n");
                        setStatusBit(SR_ERR);
                    }
                    else {
                        fseek(fd, block*BLOCK_SIZE, SEEK_SET);
                        if (fread(blockBuffer, sizeof(char), BLOCK_SIZE, fd) != BLOCK_SIZE) {
                            // printf("CF: failed to read block %d\r\n", block);
                            setStatusBit(SR_ERR);
                        }
                        else {
                            // printf("CF: Read OK\r\n");
                            setStatusBit(SR_DRQ);
                        }
                    }
                }
                clearStatusBit(SR_BSY);
            }
            break;

        case CMD_IDENTIFY_DRIVE:
            {
                int drive = (block_num >> 28) & 0x01;

                printf("CF: Execute 'Identify drive' %d\r\n", drive);
                initDriveInfo(drive);
            }
            break;

        default:
            statusReg = SR_RDY | SR_ERR;
            printf("CF: Execute unimplemented command %02x\r\n", cmd);
            break;
    }
}

void dkc::setFeatures()
{
    switch (featureReg) {
        case FEAT_ENABLE_8BIT:
            printf("CF: Feature 8-bit mode activated\r\n");
            break;

        default:
            setStatusBit(SR_ERR);
            printf("CF: Feature %02x not implemented\r\n", featureReg);
            break;
    }

    clearStatusBit(SR_BSY);
}

void dkc::clearBuffer()
{
    for (int i=0; i<BLOCK_SIZE; i++) {
        blockBuffer[i] = 0;
    }
}

void dkc::initDriveInfo(int drive)
{
    clearBuffer();
    clearStatusBit(SR_BSY | SR_ERR);
    setStatusBit(SR_DRQ);

    if (drive == 0) {
        ;
    }
}

void dkc::putWord(int offset, int val)
{
    if ((offset >= 0) && (offset < BLOCK_SIZE-1)) { 
        blockBuffer[offset] = val & 0xff;
        blockBuffer[offset+1] = (val >> 8) & 0xff;
    }
}

void dkc::putString(int offset, const char *s)
{
    int len = strlen(s);

    if ((offset >= 0) && (offset+len <= BLOCK_SIZE)) {
        while (*s) {
            blockBuffer[offset++] = *s;
        }
    }
}

void dkc::setStatusBit(int mask)
{
    statusReg |= mask;
}

void dkc::clearStatusBit(int mask)
{
    statusReg &= (~mask);
}

void dkc::checkForDriveChange(int oldDrive)
{
    int newDrive = getDriveNum();

    if (oldDrive != newDrive) {
        printf("CF: Drive bit has changed from %d to %d\r\n", oldDrive, newDrive);

        if (disks[newDrive] == NULL) {
            clearStatusBit(SR_DSC);
        }
        else {
            setStatusBit(SR_DSC);
        }
    }
}

int dkc::getDriveNum()
{
    return (block_num & 10000000)?1:0;
}