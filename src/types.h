/*
* Copyright (c) 2011-2012 by ps3dev.net
* This file is released under the GPLv2.
*/

#ifndef _TYPES_H_
#define _TYPES_H_

#include <windows.h>

typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
#ifdef _WIN32
typedef __int64 s64;
typedef unsigned __int64 u64;
#else
typedef long long int s64;
typedef unsigned long long int u64;
#endif

/*! Size of one sector. */
#define SECTOR_SIZE 0x200

#define BOOL int
#define TRUE 1
#define FALSE 0


//Endian swap for u16.
#define _ES16(val) \
	((u16)(((((u16)val) & 0xFF00) >> 8) | \
	       ((((u16)val) & 0x00FF) << 8)))

//Endian swap for u32.
#define _ES32(val) \
	((u32)(((((u32)val) & 0xFF000000) >> 24) | \
	       ((((u32)val) & 0x00FF0000) >> 8 ) | \
	       ((((u32)val) & 0x0000FF00) << 8 ) | \
	       ((((u32)val) & 0x000000FF) << 24)))

//Endian swap for u64.
#define _ES64(val) \
	((u64)(((((u64)val) & 0xFF00000000000000ULL) >> 56) | \
	       ((((u64)val) & 0x00FF000000000000ULL) >> 40) | \
	       ((((u64)val) & 0x0000FF0000000000ULL) >> 24) | \
	       ((((u64)val) & 0x000000FF00000000ULL) >> 8 ) | \
	       ((((u64)val) & 0x00000000FF000000ULL) << 8 ) | \
	       ((((u64)val) & 0x0000000000FF0000ULL) << 24) | \
	       ((((u64)val) & 0x000000000000FF00ULL) << 40) | \
	       ((((u64)val) & 0x00000000000000FFULL) << 56)))


typedef struct {
	HANDLE dev;			      // devive handle
  u8 ps3_type;					// ps3 type     layer 1(ATA)  layer 2(ENCDEC)      model
                        // 1(FAT_NAND)  aes-cbc-192   none                 CECHAxx, CECHBxx, CECHCxx, CECHExx, CECHGxx
                        // 2(FAT_NOR)   aes-cbc-192   VFLASH: aes-xts-128  CECHHxx, CECHJxx, CECHKxx, CECHLxx, CECHMxx, CECHPxx, CECHQxx
                        // 3(SLIM_NOR)  aes-xts-128   VFLASH: aes-xts-128  CECH-20xx, CECH-21xx, CECH-25xx, CECH-30xx
  u8 ata_k1[0x20];      // ATA key 1
  u8 ata_k2[0x20];      // ATA key 2
  u8 encdec_k1[0x20];   // ENCDEC key 1
  u8 encdec_k2[0x20];   // ENCDEC key 2
  u8 iv[0x10];          // AES initialization vector
  s64 vflash_start;     // vflash region start sector on HDD
  s64 vflash_size;      // vflash region sector count on HDD
  s64 hdd0_start;       // gameOS(UFS2) start sector on HDD
  s64 hdd0_size;        // gameOS(UFS2) sector count on HDD
  s64 hdd1_start;       // gameOS/swap(FAT32) start sector on HDD
  s64 hdd1_size;        // gameOS/swap(FAT32) sector count on HDD
  s64 hdd1_free;        // gameOS/swap(FAT32) free space in bytes
  s64 flash_start;      // vflash region 2(FAT16) start sector on HDD
  s64 flash_size;       // vflash region 2(FAT16) sector count on HDD
  s64 flash_free;       // vflash region 2(FAT16) free space in bytes
  s64 flash2_start;     // vflash region 3(FAT16) start sector on HDD
  s64 flash2_size;      // vflash region 3(FAT16) sector count on HDD
  s64 flash2_free;      // vflash region 3(FAT16) free space in bytes
  s64 flash3_start;     // vflash region 4(FAT12) start sector on HDD
  s64 flash3_size;      // vflash region 4(FAT12) sector count on HDD
  s64 flash3_free;      // vflash region 4(FAT12) start sector on HDD
} ps3_context;



#endif  // _TYPES_H_
