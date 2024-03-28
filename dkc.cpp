//
//
//	dkc.cpp
//
//	(C) Bob Green, 2024
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include "dkc.h"
#include "bits.h"

dkc::dkc()
{
	disks[0] = disks[1] = disks[2] = disks[3] = NULL;

	sr = SR_RDY;

	sector_num = 0;
	readBufferIndex = writeBufferIndex = 0;
	currentDiskNumber = -1;

	openDisk("disk1.img", 0);
}

dkc::~dkc()
{
}

bool dkc::openDisk(const char *diskName, int diskNum)
{
	struct stat inf;
	int res = stat(diskName, &inf);

	if (res != 0) {
		printf("\nDisk%d - '%s' not found\n", diskNum, diskName);
	}
	else {
		FILE *fd = fopen(diskName, "a+");
		if (fd != NULL) {
			long sectors;

			fseek(fd, 0L, SEEK_END);
			sectors = ftell(fd) / SECTOR_SIZE;

			printf("\nDisk%d - disk '%s' opened ok - number of sectors: %ld\n", diskNum, diskName, sectors);

			disks[diskNum] = fd;
			numSectors[diskNum] = sectors;

			return true;
		}
	}

	return false;
}

Byte dkc::read(Word offset)
{
	switch (offset & 1) {
		case 0: // status register
			printf("\r\nDKC: Read SR=0x%02x\r\n", sr);
			return sr;
			break;

		case 1:	// read data
			return 0;
			break;

		case 2: // SEC2
			return ((sector_num >> 16) & 0x0000ff);
			break;

		case 3: // SEC1
			return ((sector_num >> 8) & 0x0000ff);
			break;

		case 4: // SEC0
			return (sector_num & 0x0000ff);
			break;

		default:
			return 0xff;
			break;
	}
}

void dkc::write(Word offset, Byte val)
{
	if (!busy) {
		switch (offset & 1) {
			case 0:	// command register
				currentDiskNumber = (val >> 6) & 0x03;
				val &= 0x0f;

				sr &= ~SR_RDY;
				sr &= ~SR_ERR;

				switch (val) {
					case CMD_RESET_WRITE:
						writeBufferIndex = 0;
						sr &= ~SR_WRBF;

						// Next the code should write a sector's worth of data to the DR. The controller will assert
						// WRBF on the last byte.
						break;

					case CMD_QUERY_DRIVE:
						sector_num = numSectors[currentDiskNumber];
						if (disks[currentDiskNumber] == NULL) {
							sr |= SR_ERR;
						}
						sr |= SR_RDY;
						break;

					case CMD_WRITE:
						if (sector_num >= numSectors[currentDiskNumber]) {
							// Sector number out of range
							sr |= SR_ERR;
						}
						else {
							writeBufferIndex = 0;
						}
						break;

					case CMD_READ:
						break;

					default:
						sr |= SR_ERR;
						break;
					}
					sr |= SR_RDY;
				break;

			case 1:	// data register
				if (writeBufferIndex < SECTOR_SIZE) {
					writeBuffer[writeBufferIndex++] = val;
				}
				
				if (writeBufferIndex >= SECTOR_SIZE) {
					sr |= SR_WRBF;
				}

				// Once the buffer is full (WRBF asserted) the code should write the WRITE command to the CR
				break;

			case 2: // SEC2
				sector_num = (sector_num & 0x00ffff) | (val <<16);
				break;

			case 3: // SEC1
				sector_num = (sector_num & 0xff00ff) | (val << 8);
				break;

			case 4: // SEC0
				sector_num = (sector_num & 0xffff00) | val;
				break;
		}
	}
}
