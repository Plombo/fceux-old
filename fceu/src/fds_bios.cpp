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

extern int emulate_fds_bios;
extern int automatic_disk_change;
extern int last_disk_counter;

static int real_file_count;
uint8 *volume_header_block;
uint8 *file_count_block;
uint8 *file_header_blocks[FDS_MAX_FILES];
uint8 *file_data_blocks[FDS_MAX_FILES];

/* Parse the disk data and set up our block pointers. This only checks the
 * the block types, it does not enforce them.  Warnings will be printed and
 * -1 will be returned if something doesn't look right.
 */
/* XXX check size of disk data on every call to make sure we don't go past
 * the end, possibly into another disk image.
 */
int FDS_BIOS_OpenDisk(uint8 diskno)
{
	uint8 *disk = diskdata[diskno];
	uint8 *p = disk;
	int status = 0;
	int count = 0;
	int i;

	if (*p != FDS_BLOCK_TYPE_VOLUME_HEADER) {
		fprintf(stderr, "Found block type 0x%x; should be type 1\n",
			*p);
		status = -1;
	}

	volume_header_block = p;

	p += FDS_VOLUME_HEADER_BLOCK_SIZE;
	if (*p != FDS_BLOCK_TYPE_FILE_COUNT) {
		fprintf(stderr, "Found block type 0x%x; should be type 2\n",
			*p);
		status = -1;
	}

	file_count_block = p;
	p += FDS_FILE_COUNT_BLOCK_SIZE;

	/* Loop over file headers/data until we find a block
	 * with a type other than 3 (file header) where a
	 * header should be.
	 */
	while (*p == FDS_BLOCK_TYPE_FILE_HEADER) {
		uint8 id = p[2];
		uint8 seq = p[1];
		uint16 size = p[13] | (p[14] << 8);

		if (seq != count) {
			printf("seq != count\n");
			seq = count;
		}

		count++;
		file_header_blocks[seq] = p;
		p += FDS_FILE_HEADER_BLOCK_SIZE;

		if (*p != FDS_BLOCK_TYPE_FILE_DATA) {
			fprintf(stderr, "Found block type 0x%x; should be type 4\n",
				*p);
			status = -1;
			break;
		}

		file_data_blocks[seq] = p;
		p += size + 1;
	}

	real_file_count = count;

	/* Warn if the file count block says there are more files than we found.
	 * It is not an error for it to say there are fewer files.
	 */
	if (count < file_count_block[1]) {
		fprintf(stderr, "File count is %d, but there are only %d\n",
			file_count_block[1], count);
	}

	SelectDisk = diskno;
	if (automatic_disk_change)
		InDisk = 255;
done:
	return status;
}

void FDS_BIOS_CloseDisk(void)
{
	int i;

	volume_header_block = NULL;
	file_count_block = NULL;
	real_file_count = 0;

	for (i = 0; i < FDS_MAX_FILES; i++) {
		file_header_blocks[i] = NULL;
		file_data_blocks[i] = NULL;
	}
}

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

done:
	return status;
}

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

static int _FDS_CopyToPPU(uint16 addr, uint8 *data, size_t size)
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

static int _FDS_CheckVolumeHeader(uint8 *data)
{
	int status = FDS_STATUS_SUCCESS;

	if (data == NULL) {
		status = FDS_STATUS_NO_DISK;
		goto done;
	}

	if (*data != FDS_BLOCK_TYPE_VOLUME_HEADER) {
		status = FDS_STATUS_INVALID_VOLUME_BLOCK;
		goto done;
	}

	if (memcmp(data + 1, FDS_MAGIC, FDS_MAGIC_LENGTH)) {
		status = FDS_STATUS_INVALID_MAGIC;
		goto done;
	}
done:
	return status;
}

static int _FDS_Disk_CheckDiskID(uint8 disk, uint8 *id)
{
	int status, error;
	uint8 *current;
	int i;

	if (disk == 254 || disk == 255) {
		status = FDS_STATUS_NO_DISK;
		goto fail;
	}

	status = _FDS_CheckVolumeHeader(diskdata[disk]);
	if (status != FDS_STATUS_SUCCESS) {
		goto fail;
	}

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
	}
fail:
	return status;
}

static int _FDS_CheckDiskID(uint16 addr)
{
	uint8 id[FDS_DISKID_LENGTH];
	int status = FDS_STATUS_NO_DISK;
	uint8 disk;
	int i;

	/* Pull the requested ID from memory */
	for (i = 0; i < FDS_DISKID_LENGTH; i++) {
		id[i] = ARead[addr + i](addr + i);
	}

	if (!automatic_disk_change) {
		if (InDisk != 255)
			return _FDS_Disk_CheckDiskID(SelectDisk, id);
		else
			return FDS_STATUS_NO_DISK;
	}

	for (disk = SelectDisk + 1; disk != SelectDisk; disk++) {
		if (disk == TotalSides)
			disk = 0;
		if (_FDS_Disk_CheckDiskID(disk, id) == FDS_STATUS_SUCCESS)
			break;
	}

	if (disk < TotalSides) {
		printf("Selecting disk %d side %c\n", (disk / 2) + 1,
			disk % 2 ? 'B' : 'A');
		status = FDS_STATUS_SUCCESS;
		if (disk != SelectDisk) {
			FDS_BIOS_CloseDisk();
			FDS_BIOS_OpenDisk(disk);
		}
	}

	return status;
}

static int _FDS_GetFileCount(X6502 *xp, uint8 *count)
{
	int status = FDS_STATUS_SUCCESS;
	int handled = 1;
	uint16 disk_id_addr;
	uint16 ret_addr;

	ret_addr = GET_RETURN_ADDRESS(xp);
	disk_id_addr = GET_ADDRESS(ret_addr);
	ret_addr += 2;

	if (emulate_fds_bios) {
		status = _FDS_CheckDiskID(disk_id_addr);
		if (status != FDS_STATUS_SUCCESS) {
			goto done;
		}
	}

	if (!emulate_fds_bios) {
		handled = 0;
		goto fallthrough;
	}

	*count = file_count_block[1];
done:

	SET_RETURN_ADDRESS(xp, ret_addr);
	SET_ACCUMULATOR(xp, status);
	xp->PC = ret_addr;
	xp->S += 2;

fallthrough:

	return handled;
}

int _FDS_LoadFile(uint8 seq)
{
	uint8 type;
	uint16 address;
	uint16 size;
	int status = FDS_STATUS_SUCCESS;

	type = file_header_blocks[seq][15];
	address = file_header_blocks[seq][11] |
	          (file_header_blocks[seq][12] << 8);
	size = file_header_blocks[seq][13] |
	       (file_header_blocks[seq][14] << 8);

	if (type == 0) { /* PRG data */
		if ((address < NES_IO_PPUCTL1 &&
		    ((address & NES_RAM_MASK) < FDS_BIOS_DATA_SIZE)) ||
		    ((int)address + size > NES_RAM_TOP)) {
		   goto done;
		}
		_FDS_CopyToCPU(address, &file_data_blocks[seq][1],
	 	                 size);
	} else {
		_FDS_CopyToPPU(address, &file_data_blocks[seq][1],
	 	                 size);
	}
done:
	return status;
}

int FDS_BIOS_LoadFiles(X6502 *xp, int initial_load)
{
	uint16 disk_id_addr;
	uint16 load_list_addr;
	uint16 ret_addr;
	uint8 file_count;
	uint8 boot_id;
	uint8 files_found;
	uint8 load_list[FDS_LOAD_LIST_MAX];
	int status = FDS_STATUS_SUCCESS;
	int i, j;
	int handled = 1;

	if (!emulate_fds_bios) {
		handled = 0;
		goto fallthrough;
	}

	if (!initial_load) {
		ret_addr = GET_RETURN_ADDRESS(xp);
		disk_id_addr = GET_ADDRESS(ret_addr);
		load_list_addr = GET_ADDRESS(ret_addr + 2);
		ret_addr += 4;

		status = _FDS_CheckDiskID(disk_id_addr);
		if (status != FDS_STATUS_SUCCESS) {
			goto fail;
		}

		for(i = 0; i < FDS_LOAD_LIST_MAX; i++) {
			load_list[i] = ARead[load_list_addr + i](load_list_addr + i);
			if (load_list[i] == 0xff) {
				break;
			}
		}
	} else {
		load_list[0] = 0xff;
	}

	boot_id = volume_header_block[FDS_BOOTID_INDEX];
	file_count = file_count_block[1];

	files_found = 0;
	for (i = 0; i < file_count; i++) {
		uint8 file_id = file_header_blocks[i][2];

		if (file_header_blocks[i][0] != FDS_BLOCK_TYPE_FILE_HEADER) {
			status = FDS_STATUS_INVALID_HEADER_BLOCK;
			goto done;
		}

		if (file_data_blocks[i][0] != FDS_BLOCK_TYPE_FILE_DATA) {
			status = FDS_STATUS_INVALID_DATA_BLOCK;
			goto done;
		}

		if (load_list[0] == 0xff) {
			if (file_id > boot_id) {
				continue;
			}
		} else {
			for (j = 0; j < FDS_LOAD_LIST_MAX; j++) {
				if (load_list[j] == 0xff) {
					j = FDS_LOAD_LIST_MAX;
					break;
				} else if (load_list[j] == file_id) {
					break;
				}
			}

			if (j == FDS_LOAD_LIST_MAX) {
				continue;
			}
		}

		files_found++;
		_FDS_LoadFile(i);
	}

done:
	if (!initial_load)
		xp->Y = files_found;

	if (automatic_disk_change)
		InDisk = 255;
fail:
	if (!initial_load) {
		SET_RETURN_ADDRESS(xp, ret_addr);
		xp->S += 2;
		xp->PC = ret_addr;
		SET_ACCUMULATOR(xp, status);
	}

fallthrough:
	return handled;
}

int _FDS_SaveFile(uint8 seq, uint16 addr)
{
	uint8 *disk_offset;
	uint16 space_remaining, file_size;
	uint16 data_address;
	uint8 file_header[17];
	int status = FDS_STATUS_SUCCESS;
	int i;

	if (seq > 0) {
		uint16 prev_size;
		prev_size = file_header_blocks[seq - 1][13] |
		            (file_header_blocks[seq - 1][14] << 8);
		disk_offset = file_data_blocks[seq - 1] + 1 + prev_size;
	} else {
		disk_offset = file_count_block + FDS_FILE_COUNT_BLOCK_SIZE;
	}

	space_remaining = 65500 - (disk_offset - volume_header_block);

	/* Read in the file header data */
	for (i = 0; i < sizeof(file_header); i++) {
		file_header[i] = ARead[addr +i](addr +i);
	}

	file_size = file_header[11] | (file_header[12] << 8);

	if (file_size + 1 + FDS_FILE_HEADER_BLOCK_SIZE > space_remaining) {
		/* FIXME do we do a partial write? what error do we return? */
		fprintf(stderr, "FIXME: file too big: %u\n", file_size);
		status = FDS_STATUS_EOF;
		goto done;
	}

	data_address = file_header[14] | (file_header[15] << 8);

	/* Write the file header */
	disk_offset[0] = FDS_BLOCK_TYPE_FILE_HEADER;
	disk_offset[1] = seq;
	for (i = 0; i < FDS_FILE_HEADER_BLOCK_SIZE - 1; i++) {
		disk_offset[i + 2] = file_header[i];
	}
	file_header_blocks[seq] = disk_offset;
	disk_offset[FDS_FILE_HEADER_BLOCK_SIZE] =  FDS_BLOCK_TYPE_FILE_DATA;
	disk_offset += FDS_FILE_HEADER_BLOCK_SIZE;

	/* Write out the file data */
	/* FIXME should check results of copy*/
	if (file_header[16] == 0) {
		/* Data is located in CPU address space */
		_FDS_CopyFromCPU(disk_offset + 1, data_address,
		                          file_size);
	} else {
		_FDS_CopyFromPPU(disk_offset + 1, data_address,
		                          file_size);
	}

	file_data_blocks[seq] = disk_offset;
	DiskWritten = 1;

done:
	return status;
}

int _FDS_WriteFile(X6502 *xp, uint8 seq)
{
	uint16 disk_id_addr;
	uint16 file_header_addr;
	uint16 ret_addr;
	uint8 file_count;
	int i;
	int status;
	int handled = 1;

	ret_addr = GET_RETURN_ADDRESS(xp);
	disk_id_addr = GET_ADDRESS(ret_addr);
	file_header_addr = GET_ADDRESS(ret_addr + 2);
	ret_addr += 4;

	if (emulate_fds_bios) {
		status = _FDS_CheckDiskID(disk_id_addr);
	}

	if (!emulate_fds_bios) {
		handled = 0;
		goto fallthrough;
	}

	if (status != FDS_STATUS_SUCCESS) {
		goto done;
	}

	/* XXX real file count? */
	file_count = file_count_block[1];
	if (seq == 0xff) {
		seq = file_count;
	}

	for (i = 0; i < seq; i++) {
		if (file_header_blocks[i][0] != FDS_BLOCK_TYPE_FILE_HEADER) {
			status = FDS_STATUS_INVALID_HEADER_BLOCK;
			goto done;
		}

		if (file_data_blocks[i][0] != FDS_BLOCK_TYPE_FILE_DATA) {
			status = FDS_STATUS_INVALID_DATA_BLOCK;
			goto done;
		}
	}

	status = _FDS_SaveFile(seq, file_header_addr);
	if (status != FDS_STATUS_SUCCESS) {
		/* XXX real file count? */
		file_count_block[1] = seq;
		goto done;
	}

	/* XXX real file count? */
	file_count_block[1] = seq + 1;

	FDS_BIOS_CloseDisk();
	FDS_BIOS_OpenDisk(SelectDisk);

done:
	SET_RETURN_ADDRESS(xp, ret_addr);
	xp->PC = ret_addr;
	xp->S += 2;
	SET_ACCUMULATOR(xp, status);

fallthrough:

	return handled;
}

int FDS_BIOS_AppendFile(X6502 *xp)
{
	return _FDS_WriteFile(xp, 0xff);
}

int FDS_BIOS_WriteFile(X6502 *xp)
{
	return _FDS_WriteFile(xp, xp->A);
}

int FDS_BIOS_GetDiskInfo(X6502 *xp)
{
	uint16 ret_addr;
	uint16 disk_info_addr;
	uint8 *disk_info, *p;
	int disk_info_length;
	int disk_info_written;
	int status = FDS_STATUS_SUCCESS;
	int file_count;
	int disk_size;
	int i;
	int handled = 1;

	ret_addr = GET_RETURN_ADDRESS(xp);
	disk_info_addr = GET_ADDRESS(ret_addr);
	ret_addr += 2;
	disk_info = NULL;

	/* If we get this when we've ejected the disk and we're reporting
	 * that a disk is there anyway (for the auto change/select features),
	 * use the last selected disk since there's a good chance it's right
	 * and we have no other way of knowing which disk the game wants.
	 */

	if (!emulate_fds_bios) {
		handled = 0;
		goto fallthrough;
	}

	if (InDisk == 255) {
		status = FDS_STATUS_NO_DISK;
		goto done;
	}


	disk_info_written = 0;

	_FDS_CopyToCPU(disk_info_addr, volume_header_block + FDS_DISKID_OFFSET,
	               FDS_DISKID_LENGTH);

	disk_info_addr += FDS_DISKID_LENGTH;

	/* XXX real file count? */
	file_count = file_count_block[1];
	BWrite[disk_info_addr](disk_info_addr, file_count);
	disk_info_addr++;

	disk_info_length = (FDS_FILE_NAME_LENGTH + 1) * (file_count) + 2;
	disk_info = (uint8 *)FCEU_malloc(disk_info_length);
	if (disk_info == NULL) {
		/* FIXME */
		fprintf(stderr, "Failed to allocate memory for disk info\n");
		exit(-1);
	}

	disk_size = 0;
	p = disk_info;
	for (i = 0; i < file_count; i++) {
		int size;
		int id = file_header_blocks[i][2];
		if (id < 0 || file_header_blocks[id] == NULL) {
			continue;
		}

		if (file_header_blocks[id][0] != FDS_BLOCK_TYPE_FILE_HEADER) {
			status = FDS_STATUS_INVALID_HEADER_BLOCK;
			goto done;
		}

		*p++ = id;
		memcpy(p, &file_header_blocks[id][3], FDS_FILE_NAME_LENGTH);
		p += FDS_FILE_NAME_LENGTH;
		disk_info_written += FDS_FILE_NAME_LENGTH + 1;

		size = file_header_blocks[id][12];
		size |= file_header_blocks[id][13] << 8;

		disk_size += size;
	}

	disk_size += FDS_FILE_OVERHEAD * file_count;
	*p++ = (disk_size >> 8) & 0xff;
	*p++ = disk_size & 0xff;

	disk_info_written += 2;

done:
	if (disk_info) {
		/* FIXME check return value of this */
		_FDS_CopyToCPU(disk_info_addr, disk_info, disk_info_written);

		free(disk_info);
	}

	SET_RETURN_ADDRESS(xp, ret_addr);
	xp->PC = ret_addr;
	xp->S += 2;

	SET_ACCUMULATOR(xp, status);

fallthrough:
	return handled;
}

int FDS_BIOS_AdjustFileCount(X6502 *xp)
{
	uint8 old_count, new_count;
	int handled;

	new_count = xp->A;
	handled = _FDS_GetFileCount(xp, &old_count);

	if (!handled) {
		goto fallthrough;
	}

	if (xp->A == FDS_STATUS_SUCCESS) {
		if (new_count < old_count) {
			xp->A = FDS_STATUS_FILECOUNT_GREATER;
		}

		file_count_block[1] = old_count - new_count;
	}

	FDS_BIOS_CloseDisk();
	FDS_BIOS_OpenDisk(SelectDisk);

fallthrough:
	return handled;
}

int FDS_BIOS_CheckFileCount(X6502 *xp)
{
	uint8 old_count, new_count;
	int handled;

	new_count = xp->A;
	handled = _FDS_GetFileCount(xp, &old_count);

	if (!handled) {
		goto fallthrough;
	}

	if (xp->A == FDS_STATUS_SUCCESS) {
		if (new_count < old_count) {
			xp->A = FDS_STATUS_FILECOUNT_GREATER;
		}

		file_count_block[1] = new_count;
	}

	FDS_BIOS_CloseDisk();
	FDS_BIOS_OpenDisk(SelectDisk);

fallthrough:
	return handled;
}

int FDS_BIOS_SetFileCount1(X6502 *xp)
{
	uint8 old_count, new_count;
	int handled;

	new_count = xp->A;
	handled = _FDS_GetFileCount(xp, &old_count);

	if (!handled) {
		goto fallthrough;
	}

	if (xp->A == FDS_STATUS_SUCCESS) {
		file_count_block[1] = new_count;
	}

	FDS_BIOS_CloseDisk();
	FDS_BIOS_OpenDisk(SelectDisk);

fallthrough:
	return handled;
}

int FDS_BIOS_SetFileCount2(X6502 *xp)
{
	uint8 old_count, new_count;
	int handled;

	new_count = xp->A;
	handled = _FDS_GetFileCount(xp, &old_count);

	if (!handled) {
		goto fallthrough;
	}

	if (xp->A == FDS_STATUS_SUCCESS) {
		file_count_block[1] = new_count + 1;
	}

	FDS_BIOS_CloseDisk();
	FDS_BIOS_OpenDisk(SelectDisk);

fallthrough:
	return handled;
}

int FDS_BIOS_CheckDiskHeader(X6502 *xp)
{
	uint8 status;
	uint16 addr;
	uint16 ret_addr;

	ret_addr = GET_RETURN_ADDRESS(xp);

	addr = ((uint16)RAM[0x01] << 8) | RAM[0x00];

	if (!automatic_disk_change && InDisk == 255)
		status = FDS_STATUS_NO_DISK;
	else 
		status = _FDS_CheckDiskID(addr);

	//SET_RETURN_ADDRESS(xp, ret_addr);
	xp->PC = ret_addr;
	xp->S += 2;

	SET_ACCUMULATOR(xp, status);

	return 1;
}

/* XXX return error if no disk in manual mode */
int FDS_BIOS_GetFileCount(X6502 *xp)
{
	uint8 file_count;
	uint16 ret_addr;

	ret_addr = GET_RETURN_ADDRESS(xp);
	file_count = file_count_block[1];

	RAM[0x06] = file_count;
	//SET_RETURN_ADDRESS(xp, ret_addr);
	xp->PC = ret_addr;
	xp->S += 2;

	SET_ACCUMULATOR(xp, file_count);

	return 1;
}

/* XXX return error if no disk in manual mode */
int FDS_BIOS_CheckFileType(X6502 *xp)
{
	uint8 file_type;
	uint16 ret_addr;

	ret_addr = GET_RETURN_ADDRESS(xp);
	file_type = xp->A;

	RAM[0x07] = file_type;

	xp->PC = ret_addr;
	xp->S += 2;

	//xp->X = file_type + 0x21;
	//xp->Y = xp->X;

	SET_ACCUMULATOR(xp, 0);

	return 1;
}

/* XXX return error if no disk in manual mode */
int FDS_BIOS_FileMatchTest(X6502 *xp)
{
	uint16 load_list_addr;
	uint16 ret_addr;
	uint8 boot_id;
	uint8 file_id, current_id; 
	uint8 current_file;
	int i;
	int load_file;

	current_file = real_file_count - RAM[0x06];
	ret_addr = GET_RETURN_ADDRESS(xp);
	load_list_addr = ((uint16)RAM[0x03] << 8) | RAM[0x02];
	boot_id = volume_header_block[FDS_BOOTID_INDEX];

	current_id = file_header_blocks[current_file][2];
	load_file = 0;
	for(i = 0; i < FDS_LOAD_LIST_MAX; i++) {
		file_id = ARead[load_list_addr + i](load_list_addr + i);
		if (file_id == 0xff && i == 0 && current_id <= boot_id) {
			load_file = 1;
			break;
		} else if (file_id == current_id) {
			load_file = 1;
			break;
		} else if (file_id == 0xff) {
			break;
		}
	}

	if (load_file) {
		RAM[0x09] = 0;
		RAM[0x0e]++;
	} else {
		RAM[0x09] = 0xff;
	}

	xp->PC = ret_addr;
	xp->S += 2;

	SET_ACCUMULATOR(xp, 0);

	return 1;
}

int FDS_BIOS_FileLoad(X6502 *xp)
{
	uint16 ret_addr;
	int i;
	int load_file;
	int status = FDS_STATUS_SUCCESS;
	uint8 current_file;

	ret_addr = GET_RETURN_ADDRESS(xp);

	if (!automatic_disk_change && InDisk == 255) {
		status = FDS_STATUS_NO_DISK;
	} else if (RAM[0x09] == 0) {
		current_file = real_file_count - RAM[0x06];
		status = _FDS_LoadFile(current_file);
	}

	xp->PC = ret_addr;
	xp->S += 2;

	SET_ACCUMULATOR(xp, status);

	return 1;
}
