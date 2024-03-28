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
		const static int MAX_DISKS = 4;
		const static int SECTOR_SIZE = 512;

		const static Byte CMD_WRITE 		= 0x01;
		const static Byte CMD_READ  		= 0x02;
		const static Byte CMD_QUERY_DRIVE 	= 0x03; 
		const static Byte CMD_RESET_WRITE 	= 0x04; 

		const static Byte SR_ERR  = 0x01;
		const static Byte SR_RDBF = 0x02;
		const static Byte SR_WRBF = 0x04;
		const static Byte SR_RDY  = 0x80;

	// Internal registers
	protected:
		Byte				cmd, sr, cr, dr;
		Byte				drive_num;
		int32_t				sector_num;
		bool				busy;
		FILE				*disks[MAX_DISKS];
		int					numSectors[MAX_DISKS];
		Byte				readBuffer[SECTOR_SIZE];
		Byte				writeBuffer[SECTOR_SIZE];
		int					readBufferIndex;
		int 				writeBufferIndex;
		int					currentDiskNumber;

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

};
