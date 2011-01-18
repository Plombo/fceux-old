#ifndef __FDS_BIOS_H__
#define __FDS_BIOS_H__

#include "types.h"
#include "x6502.h"

#define FDS_STATUS_SUCCESS			0x00
#define FDS_STATUS_NO_DISK			0x01
#define FDS_STATUS_POWER_FAIL			0x02
#define FDS_STATUS_WRITE_PROTECTED		0x03
#define FDS_STATUS_INVALID_MFG			0x04
#define FDS_STATUS_INVALID_NAME			0x05
#define FDS_STATUS_INVALID_VERSION		0x06
#define FDS_STATUS_INVALID_SIDE			0x07
#define FDS_STATUS_INVALID_DISK			0x08
#define FDS_STATUS_INVALID_EXTRA1		0x09
#define FDS_STATUS_INVALID_EXTRA2		0x10
#define FDS_STATUS_INVALID_MAGIC		0x21
#define FDS_STATUS_INVALID_VOLUME_BLOCK		0x22
#define FDS_STATUS_INVALID_FILE_COUNT_BLOCK	0x23
#define FDS_STATUS_INVALID_HEADER_BLOCK		0x24
#define FDS_STATUS_INVALID_DATA_BLOCK		0x25
#define FDS_STATUS_WRITE_VERIFY_FAILED		0x26
#define FDS_STATUS_INVALID_CRC			0x27
#define FDS_STATUS_SHORT_READ			0x28
#define FDS_STATUS_SHORT_WRITE			0x29
#define FDS_STATUS_EOF				0x30
#define FDS_STATUS_FILECOUNT_GREATER		0x31

/* This is only used by the HLE code; the real ROM doesn't
 * ever generate this (which is why it's two bytes instead
 * of one). */
#define FDS_STATUS_NMI_HACK			0x100

#define FDS_DISKID_LENGTH			10
#define FDS_DISKID_OFFSET			15
#define FDS_MAX_FILES				256
#define FDS_LOAD_LIST_MAX			20

#define FDS_BLOCK_TYPE_VOLUME_HEADER		0x1
#define FDS_BLOCK_TYPE_FILE_COUNT		0x2
#define FDS_BLOCK_TYPE_FILE_HEADER		0x3
#define FDS_BLOCK_TYPE_FILE_DATA		0x4

#define FDS_VOLUME_HEADER_BLOCK_SIZE		56
#define FDS_FILE_COUNT_BLOCK_SIZE		2
#define FDS_FILE_HEADER_BLOCK_SIZE		16

#define NES_RAM_SIZE				0x800
#define NES_RAM_MASK				0x7FF
#define NES_RAM_TOP				0x10000
#define NES_IO_PPUCTL1				0x2000

#define FDS_RAM_ADDRESS				0x6000
#define FDS_RAM_SIZE				0x8000
#define FDS_ROM_ADDRESS				0xE000
#define FDS_ROM_SIZE				0x2000
#define FDS_BIOS_DATA_SIZE			0x200

#define FDS_MAGIC				"*NINTENDO-HVC*"
#define FDS_MAGIC_LENGTH			14
#define FDS_BOOTID_INDEX			25

#define FDS_FILE_NAME_LENGTH			8
#define FDS_FILE_OVERHEAD			261

#define FDS_BIOS_NMI_HANDLER		0xE18B
#define FDS_BIOS_LOAD_FILES		0xE1F8
#define FDS_BIOS_APPEND_FILE		0xE237
#define FDS_BIOS_WRITE_FILE		0xE239
#define FDS_BIOS_GET_DISK_INFO		0xE32A
#define FDS_BIOS_ADJUST_FILE_COUNT	0xE2BB
#define FDS_BIOS_CHECK_FILE_COUNT	0xE2B7
#define FDS_BIOS_SET_FILE_COUNT2	0xE301
#define FDS_BIOS_SET_FILE_COUNT1	0xE305
#define FDS_BIOS_CHECK_DISK_HEADER	0xE445
#define FDS_BIOS_GET_FILE_COUNT		0xE484
#define FDS_BIOS_FILE_MATCH_TEST	0xE4A0
#define FDS_BIOS_FILE_LOAD		0xE4F9
#define FDS_BIOS_CHECK_FILE_TYPE	0xE68F

#define FDS_MAX_INSERT_COUNTER        32


int FDS_BIOS_OpenDisk(uint8);
void FDS_BIOS_CloseDisk(void);
int FDS_BIOS_LoadFiles(X6502 *xp, int initial_load);
int FDS_BIOS_AppendFile(X6502 *xp);
int FDS_BIOS_WriteFile(X6502 *xp);
int FDS_BIOS_GetDiskInfo(X6502 *xp);
int FDS_BIOS_AdjustFileCount(X6502 *xp);
int FDS_BIOS_CheckFileCount(X6502 *xp);
int FDS_BIOS_SetFileCount1(X6502 *xp);
int FDS_BIOS_SetFileCount2(X6502 *xp);
int FDS_BIOS_CheckDiskHeader(X6502 *xp);
int FDS_BIOS_GetFileCount(X6502 *xp);
int FDS_BIOS_CheckFileType(X6502 *xp);
int FDS_BIOS_FileMatchTest(X6502 *xp);
int FDS_BIOS_FileLoad(X6502 *xp);

#endif /* __FDS_BIOS_H__ */
