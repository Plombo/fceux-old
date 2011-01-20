#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "x6502.h"
#include "fds.h"
#include "fds_bios.h"
#include "utils/memory.h"
#include "fceu.h"
#include "ppu.h"

#define GET_RETURN_ADDRESS(x) ((RAM[0x100 + (x)->S + 1] | (RAM[0x100 + (x)->S + 2] << 8)) + 1)
#define SET_RETURN_ADDRESS(x, a) { \
	RAM[0x100 + (x)->S + 1] = (a - 1);\
       	RAM[0x100 + (x)->S + 2] = ((a - 1) >> 8); \
	}

#define GET_ADDRESS(a) (ARead[(a)]((a)) | (ARead[(a) + 1]((a) + 1) << 8))

#define SET_ACCUMULATOR(xp, v) { \
	(v) ? (xp)->P &= ~Z_FLAG : (xp)->P |= Z_FLAG; \
	(v & 0x80) ? (xp)->P |= N_FLAG : (xp)->P &= ~N_FLAG; \
	(xp)->A = v; \
	}

#define FDSRAM GameMemBlock
#define CHRRAM (GameMemBlock+32768)

#define PPU_RAM_MASK		0x3FFF
#define PALETTE_MASK		0x3F1F
#define NAME_TABLE_MASK		0x2EFF

#define NAME_TABLE_0		0x2000
#define NAME_TABLE_1		0x2400
#define NAME_TABLE_2		0x2800
#define NAME_TABLE_3		0x2C00
#define NAME_TABLE_END		0x3000
#define PALETTE_START		0x3F00
#define PALETTE_SIZE		0x0020

#define HANDLE_LOAD_FILES
#define HANDLE_WRITE_FILE

extern uint8 PALRAM[0x20];
extern uint8 *diskdata[8];
extern uint8 InDisk,SelectDisk;
extern uint8 FDSBIOS[8192];
extern int TotalSides;
extern int DiskWritten;
extern uint32 DiskPtr;

extern int emulate_fds_bios;
extern int automatic_disk_change;

/* Copy data to address in 6502-space.  Works like memcpy, except
 * it will only copy to actual RAM; it won't write to FDS ROM area
 * or I/O registers.  Data is truncated to fit in RAM if it would
 * overflow into these areas. The mirroring of main NES RAM is taken
 * into account.
 *
 * addr must be within a valid RAM address range: 0x0000 - 0x0200,
 * 0x6000 - 0xE000.
 *
 * Returns the number of bytes copied (which may be less than size
 * if data won't fit into RAM). If you need to write to I/O registers,
 * don't use this.
 */
ssize_t _FDS_CopyToCPU(uint16 addr, uint8 *data, size_t size)
{
	int status = -1;
	int chunk_size;
	int limit;
	size_t rest;

	printf("orig size: %d addr: %x\n", size, addr);

	rest = size;
	while (rest > 0) {
		chunk_size = rest;
		if (addr < NES_IO_PPUCTL1) {
			uint16 real_addr = addr & NES_RAM_MASK;
			limit = NES_RAM_SIZE;
			if ((real_addr + rest) > NES_RAM_SIZE) {
				chunk_size = NES_RAM_SIZE - real_addr;
			}

			memcpy(RAM + real_addr, data, chunk_size);
		} else if (addr < FDS_RAM_ADDRESS) {
			if (addr + rest > FDS_RAM_ADDRESS) {
				chunk_size = FDS_RAM_ADDRESS - addr;
			}
		} else if (addr < FDS_ROM_ADDRESS) {
			if (addr + rest > FDS_ROM_ADDRESS) {
				chunk_size = FDS_ROM_ADDRESS - addr;
			}

			memcpy(FDSRAM + addr - FDS_RAM_ADDRESS, data, chunk_size);
		} else if (addr < NES_RAM_TOP) {
			if (addr + rest > NES_RAM_TOP) {
				chunk_size = NES_RAM_TOP - addr;
			}
		}

		addr += chunk_size;
		data += chunk_size;
		rest -= chunk_size;
	}

	status = 0;

done:
	return status;
}

/* This works like memcpy, but copies data from the specified address in
 * the 2A03's address space.
 */
ssize_t _FDS_CopyFromCPU(uint8 *data, uint16 addr, size_t size)
{
	int status = -1;
	int chunk_size;
	int limit;
	size_t rest;

	rest = size;
	while (rest > 0) {
		chunk_size = size;
		if (addr < NES_IO_PPUCTL1) {
			uint16 real_addr = addr & NES_RAM_MASK;
			limit = NES_RAM_SIZE;
			if ((real_addr + rest) > NES_RAM_SIZE) {
				chunk_size = NES_RAM_SIZE - real_addr;
			}

			memcpy(data, RAM + real_addr, chunk_size);
		} else if (addr < FDS_RAM_ADDRESS) {
			if (addr + rest > FDS_RAM_ADDRESS) {
				chunk_size = FDS_RAM_ADDRESS - addr;
			}

			memset(data, 0, chunk_size);
		} else if (addr < FDS_ROM_ADDRESS) {
			if (addr + rest > FDS_ROM_ADDRESS) {
				chunk_size = FDS_ROM_ADDRESS - addr;
			}

			memcpy(data, FDSRAM + addr - FDS_RAM_ADDRESS, chunk_size);
		} else if (addr < NES_RAM_TOP) {
			if (addr + rest > NES_RAM_TOP) {
				chunk_size = NES_RAM_TOP - addr;
			}

			memcpy(data, FDSBIOS + addr - FDS_ROM_ADDRESS, chunk_size);
		}

		addr += chunk_size;
		data += chunk_size;
		rest -= chunk_size;
	}

done:
	return status;
}

/* This copies data to the PPU address space */
static int _FDS_CopyToPPU(uint16 addr, uint8 *data, size_t size)
{
	uint16 start_addr, end_addr;
	int chunk_size;
	uint8 *ptr;
	int status;

	status = FDS_STATUS_SUCCESS;

#if 0
	BWrite[0x2006](0x2006, (addr >> 8) & 0xff);
	BWrite[0x2006](0x2006, addr & 0xff);
	while (size > 0) {
		BWrite[0x2007](0x2007, *data);
		data++;
		size--;
	}
#else
	while (size > 0) {
		addr &= PPU_RAM_MASK;

		if (addr >= NAME_TABLE_END && addr < PALETTE_START) {
			addr &= NAME_TABLE_MASK;
		}
		else if (addr > PALETTE_START) {
			addr &= PALETTE_MASK;
		}

		if (addr < NAME_TABLE_0) { /* Pattern table */
			start_addr = 0;
			end_addr = NAME_TABLE_0;
			ptr = CHRRAM;
		} else if (addr < NAME_TABLE_1) { /* Name Table 0 */
			start_addr = NAME_TABLE_0;
			end_addr = NAME_TABLE_1;
			ptr = vnapage[0];
		} else if (addr < NAME_TABLE_2) { /* Name Table 1 */
			start_addr = NAME_TABLE_1;
			end_addr = NAME_TABLE_2;
			ptr = vnapage[1];
		} else if (addr < NAME_TABLE_3) { /* Name Table 2 */
			start_addr = NAME_TABLE_2;
			end_addr = NAME_TABLE_3;
			ptr = vnapage[2];
		} else if (addr < NAME_TABLE_END) { /* Name Table 3 */
			start_addr = NAME_TABLE_3;
			end_addr = NAME_TABLE_END;
			ptr = vnapage[3];
		} else if (addr > PALETTE_START) {
			start_addr = PALETTE_START;
			end_addr = PALETTE_START + PALETTE_SIZE;
			ptr = PALRAM;
		}

		chunk_size = size;
		if (size > end_addr - addr) {
			chunk_size = end_addr - addr;
		}

		memcpy(ptr + addr - start_addr, data, chunk_size);
		addr += chunk_size;
		data += chunk_size;
		size -= chunk_size;
	}
#endif

	return status;
}

static int _FDS_CopyFromPPU(uint8 *data, uint16 addr, size_t size)
{
	uint16 start_addr, end_addr;
	int chunk_size;
	uint8 *ptr;
	int status;

	status = FDS_STATUS_SUCCESS;

	while (size > 0) {
		addr &= PPU_RAM_MASK;

		if (addr >= NAME_TABLE_END && addr < PALETTE_START) {
			addr &= NAME_TABLE_MASK;
		}
		else if (addr > PALETTE_START) {
			addr &= PALETTE_MASK;
		}

		if (addr < NAME_TABLE_0) { /* Pattern table */
			start_addr = 0;
			end_addr = NAME_TABLE_0;
			ptr = CHRRAM;
		} else if (addr < NAME_TABLE_1) { /* Name Table 0 */
			start_addr = NAME_TABLE_0;
			end_addr = NAME_TABLE_1;
			ptr = vnapage[0];
		} else if (addr < NAME_TABLE_2) { /* Name Table 1 */
			start_addr = NAME_TABLE_1;
			end_addr = NAME_TABLE_2;
			ptr = vnapage[1];
		} else if (addr < NAME_TABLE_3) { /* Name Table 2 */
			start_addr = NAME_TABLE_2;
			end_addr = NAME_TABLE_3;
			ptr = vnapage[2];
		} else if (addr < NAME_TABLE_END) { /* Name Table 2 */
			start_addr = NAME_TABLE_3;
			end_addr = NAME_TABLE_END;
			ptr = vnapage[3];
		} else if (addr > PALETTE_START) {
			start_addr = PALETTE_START;
			end_addr = PALETTE_START + PALETTE_SIZE;
			ptr = PALRAM;
		}

		chunk_size = size;
		if (size > end_addr - addr) {
			chunk_size = end_addr - addr;
		}

		memcpy(data, ptr + addr - start_addr, chunk_size);
		addr += chunk_size;
		data += chunk_size;
		size -= chunk_size;
	}

	return status;
}

/* This checks the specified disk's ID block with the one passed in
 * the ID parameter.  For two the id to match, each byte must be equal
 * to the corresponding byte in the on-disk ID OR be equal to 0xff.  If
 * the ID didn't match, return the index of the first non-matching byte
 * (unless it is the title name that doesn't match, in which case 0
 * will be returned).
 */
static int _FDS_Disk_CheckDiskID(uint8 disk, uint8 *id)
{
	int status, error;
	uint8 *current;
	int i;

	if (disk == 254 || disk == 255) {
		status = FDS_STATUS_NO_DISK;
		goto fail;
	}

	/* FIXME does this really need to be checked here? */
	if (diskdata[disk][FDS_VOLUME_HEADER_BLOCK_SIZE] !=
	    FDS_BLOCK_TYPE_FILE_COUNT) {
		status = FDS_STATUS_INVALID_FILE_COUNT_BLOCK;
		goto fail;
	}

	current = diskdata[disk] + FDS_DISKID_OFFSET;
	error = FDS_STATUS_INVALID_MFG;

	/* compare the two ids */
	for (i = 0; i < FDS_DISKID_LENGTH; i++) {
		if (id[i] != 0xff && id[i] != current[i]) {
			break;
		}

		/* Increment the error to return, but don't increment
		 * for each byte in the name.
		 */
		if (i == 0 || i >= 4) {
			error++;
		}
	}

	if (i < FDS_DISKID_LENGTH) {
		status = error;
	} else {
		status = FDS_STATUS_SUCCESS;
	}
fail:
	return status;
}

int FDS_BIOS_EndOfBlkRead(X6502 *xp)
{
	uint16 ret_addr;

	ret_addr = GET_RETURN_ADDRESS(xp);

	xp->PC = ret_addr;

	xp->S += 2;

	return 1;
}

int FDS_BIOS_TransferByte(X6502 *xp)
{
	uint16 ret_addr;
	uint8 value;

	ret_addr = GET_RETURN_ADDRESS(xp);

	xp->X = diskdata[SelectDisk][DiskPtr];
	value = xp->A;
	xp->A = xp->X;
	xp->P &= ~I_FLAG;

//	printf("read byte %hx from offset %d\n", xp->X, DiskPtr);

	if (DiskPtr < 64999)
		DiskPtr++;

	xp->PC = ret_addr;
	xp->S += 2;
	
	return 1;
}

int FDS_BIOS_LoadPRG_RAM(X6502 *xp)
{
	uint16 size;
	uint16 dest_addr;
	int status;

	size = (RAM[0x0d] << 8 | RAM[0x0c]) + 1;
	dest_addr = RAM[0x0b] << 8 | RAM[0x0a];

	if (dest_addr >= NES_IO_PPUCTL1 && dest_addr < FDS_RAM_ADDRESS)
		return 0;
	else if (dest_addr < NES_IO_PPUCTL1 && (dest_addr + size) >=
		 NES_IO_PPUCTL1)
		return 0;

	status = _FDS_CopyToCPU(dest_addr,
			      (uint8 *)&diskdata[SelectDisk][DiskPtr],
			      size);

	if (status >= 0) {
		DiskPtr += size;
		dest_addr += size;
		RAM[0x0c] = RAM[0x0d] = 0xff;
		RAM[0x0b] = (dest_addr >> 8) & 0xff;
		RAM[0x0a] = (dest_addr & 0x0f);
		xp->PC = 0xe572;
		return 1;
	}

	return 0;
}


int FDS_BIOS_LoadCHR_RAM(X6502 *xp)
{
	uint16 size;
	uint16 dest_addr;
	int status;

	size = (RAM[0x0d] << 8 | RAM[0x0c]) + 1;
	dest_addr = RAM[0x0b] << 8 | RAM[0x0a];

	status = _FDS_CopyToPPU(dest_addr,
			      (uint8 *)&diskdata[SelectDisk][DiskPtr],
			      size);

	if (status == FDS_STATUS_SUCCESS) {
		DiskPtr += size;
		dest_addr += size;
		RAM[0x0c] = RAM[0x0d] = 0xff;
		RAM[0x0b] = (dest_addr >> 8) & 0xff;
		RAM[0x0a] = (dest_addr & 0x0f);
		xp->PC = 0xe572;
		return 1;
	}

	return 0;
}


int FDS_BIOS_AutoDiskSelect(X6502 *xp)
{
	uint8 id[FDS_DISKID_LENGTH];
	uint8 disk;
	uint16 addr;
	int matches;
	int i;

	addr = RAM[0x01] << 8 | RAM[0x00];

	/* Pull the requested ID from memory */
	for (i = 0; i < FDS_DISKID_LENGTH; i++) {
		id[i] = ARead[addr + i](addr + i);
	}

	matches = 0;
	for (i = 0; i < TotalSides; i++) {
		if (_FDS_Disk_CheckDiskID(i, id) == FDS_STATUS_SUCCESS) {
			disk = i;
			matches++;
		}
	}

	if (matches > 1 || matches == 0)
		return 0;

	if (disk != SelectDisk) {
		SelectDisk = disk;
		InDisk = SelectDisk;
		FCEU_DispMessage("Disk %d Side %c Auto-Selected", 0, (SelectDisk>>1) + 1,
		                 (SelectDisk&1)?'B':'A');
	}

	return 1;
}
