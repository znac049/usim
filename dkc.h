//
//
//	dkc.h
//
//	(C) Bob Green, 2024
//

#pragma once

#include "device.h"
#include "wiring.h"

class dkc : virtual public MappedDevice {
	protected:
		const static int MAX_DISKS = 2;
		const static int BLOCK_SIZE = 512;

        // ATA CF device registers
        const static int CF_Data        = 0; // read/write
        const static int CF_Error       = 1; // read
        const static int CF_Features    = 1; // write
        const static int CF_SecCnt      = 2; // read/write
        const static int CF_LSN0        = 3; // read/write - bits  0-7
        const static int CF_LSN1        = 4; // read/write - bits  8-15
        const static int CF_LSN2        = 5; // read/write - bits 16-23
        const static int CF_LSN3        = 6; // read/write - bits 24-27
        const static int CF_Status      = 7; // read
        const static int CF_Command     = 7; // write

        // Commands
        const static int CMD_RESET            = 0x04;
        const static int CMD_READ_SECTORS     = 0x20;        
        const static int CMD_READ_SECTORS_ALT = 0x21;        
        const static int CMD_DIAG             = 0x90;
        const static int CMD_IDENTIFY_DRIVE   = 0xec;
        const static int CMD_SET_FEATURES     = 0xef;

        // Bits in the status register
        const static int SR_ERR  = 0x01;
        const static int SR_IDX  = 0x02;
        const static int SR_CORR = 0x04;
        const static int SR_DRQ  = 0x08;
        const static int SR_DSC  = 0x10;
        const static int SR_DWF  = 0x20;
        const static int SR_RDY  = 0x40;
        const static int SR_BSY  = 0x80;

        // Features
        const static int FEAT_ENABLE_8BIT = 0x01;

	// Internal registers
	protected:
		int32_t				block_num;
		FILE				*disks[MAX_DISKS];
		int					numBlocks[MAX_DISKS];
        Byte                blockBuffer[BLOCK_SIZE];
        int                 readIndex;

        Byte                errorReg;
        Byte                featureReg;
        Byte                sectorCountReg;
        Byte                statusReg;

	// Initialisation functions
	bool openDisk(const char *name, int diskNum);

	// Read and write functions
	public:
		virtual Byte		read(Word offset);
		virtual void		write(Word offset, Byte val);

	// Other exposed interfaces
	public:

	// Public constructor and destructor
							dkc();
		virtual				~dkc();

    private:
        void reset(void);
        void cfCommand(Byte cmd);
        void setFeatures();
        void clearBuffer();
        void initDriveInfo(int drive);

        void putWord(int, int);
        void putString(int, const char *);

        void setStatusBit(int);
        void clearStatusBit(int);
        void checkForDriveChange(int);
        int getDriveNum();
};
