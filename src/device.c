/*
* Copyright (c) 2019 by 3141card
* This file is released under the GPLv2.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "device.h"

// decryption contexts
static aes_context    cbc_dec;
static aes_xts_ctxt_t xts_dec;
static aes_xts_ctxt_t xts_dec_vf;

// encryption contexts
static aes_context    cbc_enc;
static aes_xts_ctxt_t xts_enc;
static aes_xts_ctxt_t xts_enc_vf;



/***********************************************************************
* Is region/partition VFLASH? Set start and size of regions if yes.
* 
* ps3_context *ctx = ps3 device information
* u8 *buf          = start of region/partition to test
***********************************************************************/
static void check_partition_vflash(ps3_context *ctx, u8 *buf)
{
	struct disklabel *pt = (struct disklabel *)buf;
  
  if((_ES64(pt->d_magic1) == MAGIC1) && (_ES64(pt->d_magic2) == MAGIC2)) {
    // vflash 2(FAT16)
    ctx->flash_start  = _ES64(pt->d_partitions[1].p_start) + ctx->vflash_start;
    ctx->flash_size   = _ES64(pt->d_partitions[1].p_size);
	  // vflash 3(FAT16)
	  ctx->flash2_start = _ES64(pt->d_partitions[2].p_start) + ctx->vflash_start;
	  ctx->flash2_size  = _ES64(pt->d_partitions[2].p_size);
	  // vflash 4(FAT12)
	  ctx->flash3_start = _ES64(pt->d_partitions[3].p_start) + ctx->vflash_start;	
	  ctx->flash3_size  = _ES64(pt->d_partitions[3].p_size);
	}
}

/***********************************************************************
* Is region/partition gameOS? Set start and size if yes.
* 
* ps3_context *ctx = ps3 device information
* u8 *buf          = start of region/partition to test
***********************************************************************/
static void check_partition_gameOS(ps3_context *ctx, u8 *buf, u64 start, u64 size)
{
  struct fs *ufs2 = (struct fs *)(buf + SBLOCK_UFS2);
  
  if(_ES32(ufs2->fs_magic) == FS_UFS2_MAGIC) {
    ctx->hdd0_start = start;
    ctx->hdd0_size  = size;
  }
}

/***********************************************************************
* Is region/partition gameOS_swap? Set start and size if yes.
* 
* ps3_context *ctx = ps3 device information
* u8 *buf          = start of region/partition to test
***********************************************************************/
static void check_partition_gameOS_swap(ps3_context *ctx, u8 *buf, u64 start, u64 size)
{
  struct fat32_info *info = (struct fat32_info *)(buf + SECTOR_SIZE);
  if((info->i_m1 == FAT32_INFO_SIG_1) && (info->i_m2 == FAT32_INFO_SIG_2)) {
    ctx->hdd1_start = start;
    ctx->hdd1_size  = size;
  }
}

/***********************************************************************
* Is device a ps3 hdd/image? Get information if yes.
* 
* ps3_context *ctx = ps3 device information
***********************************************************************/
static s32 check_device(ps3_context *ctx)
{
	DWORD n_read = 0;
	u8 sec_0[SECTOR_SIZE] = {};
	u8 sec_8[SECTOR_SIZE] = {};
	u8 tmp[SECTOR_SIZE] = {};
	struct disklabel *pt_hdd;
	struct disklabel *pt_vflash;
	
	// get sector 0(first of ps3 main partition table)
	// and sector 8(first of VFLASH partition table, if a VFLASH)
  seek_device(ctx->dev, 0);
  ReadFile(ctx->dev, sec_0, SECTOR_SIZE * 1, &n_read, 0);
  _es16_buffer(sec_0, SECTOR_SIZE * 1);
  
  seek_device(ctx->dev, 8);
  ReadFile(ctx->dev, sec_8, SECTOR_SIZE * 1, &n_read, 0);
  _es16_buffer(sec_8, SECTOR_SIZE * 1);
	
	// decrypt sector 0 layer 1(ATA) with aes_cbc_192
	memcpy(tmp, sec_0, SECTOR_SIZE);
	aes_setkey_dec(&cbc_dec, ctx->ata_k1, 192);
	aes_crypt_cbc(&cbc_dec, AES_DECRYPT, SECTOR_SIZE, ctx->iv, tmp, tmp);
	memset(ctx->iv, 0, 16);
	
	pt_hdd = (struct disklabel *)tmp;
  
  // is partition table magic ?
	if(_ES64(pt_hdd->d_magic1) == MAGIC1 && _ES64(pt_hdd->d_magic2) == MAGIC2) {
    // layer 1(ATA) is aes-cbc-192: ps3_type == 1(FAT_NAND) or 2(FAT_NOR)
    // check sector 8 for VFLASH:
    memcpy(tmp, sec_8, SECTOR_SIZE);
		    
    // decrypt sector 8 layer 1(ATA) with aes_cbc_192
    aes_setkey_dec(&cbc_dec, ctx->ata_k1, 192);
    aes_crypt_cbc(&cbc_dec, AES_DECRYPT, SECTOR_SIZE, ctx->iv, tmp, tmp);
    memset(ctx->iv, 0, 16);
    
    // decrypt sector 8 layer 2(VFLASH) with aes-xts-128
    aes_xts_init(&xts_dec, AES_DECRYPT, ctx->encdec_k1, ctx->encdec_k2, 128);
    aes_xts_crypt(&xts_dec, 8, SECTOR_SIZE, tmp, tmp);
    
    pt_vflash = (struct disklabel *)tmp;
    
		// is partition table magic ?
		if((_ES64(pt_vflash->d_magic1) == MAGIC1) && (_ES64(pt_vflash->d_magic2) == MAGIC2)) {
			// first region starts with a partition table; ps3_type == 1(FAT_NOR)
			ctx->ps3_type = 2;
			
			// set VFLASH start and size
			ctx->vflash_start = _ES64(pt_hdd->d_partitions[0].p_start);
      ctx->vflash_size  = _ES64(pt_hdd->d_partitions[0].p_size);
      
      // set encryption context for layer 2(VFLASH) 
      aes_xts_init(&xts_enc_vf, AES_ENCRYPT, ctx->encdec_k1, ctx->encdec_k2, 128);
		}
		else {
		  // no partition table for first region; ps3_type == 1(FAT_NAND)
		  ctx->ps3_type = 1;
		}
		// set encryption context for layer 1(ATA) 
		aes_setkey_enc(&cbc_enc, ctx->ata_k1, 192);
		
	  return 0; 
	}
	
	// not a FAT, test sector 0 layer 1(ATA) with aes_xts_128
  memcpy(tmp, sec_0, SECTOR_SIZE);
  aes_xts_init(&xts_dec, AES_DECRYPT, ctx->ata_k1, ctx->ata_k2, 128);
  aes_xts_crypt(&xts_dec, 0, SECTOR_SIZE, tmp, tmp);
	
	pt_hdd = (struct disklabel *)tmp;
	
	if(_ES64(pt_hdd->d_magic1) == MAGIC1 && _ES64(pt_hdd->d_magic2) == MAGIC2) {
		// layer 1(ATA) is aes_xts_128; ps3_type == 3(SLIM_NOR)
		ctx->ps3_type = 3;
		
		// set VFLASH start and size
		ctx->vflash_start = _ES64(pt_hdd->d_partitions[0].p_start);
    ctx->vflash_size  = _ES64(pt_hdd->d_partitions[0].p_size);
    
    // set layer 2(VFLASH) decrytion context
    aes_xts_init(&xts_dec_vf, AES_DECRYPT, ctx->encdec_k1, ctx->encdec_k2, 128);
    
    // set encryption contexts for layer 1(ATA) and layer 2(VFLASH)
    aes_xts_init(&xts_enc, AES_ENCRYPT, ctx->ata_k1, ctx->ata_k2, 128);
	  aes_xts_init(&xts_enc_vf, AES_ENCRYPT, ctx->encdec_k1, ctx->encdec_k2, 128);
	  
		return 0;
	}
	
	// either no ps3 hdd or the wrong eid root key
	return -1;
}

/***********************************************************************
* Get ps3 hdd/file handle.
* 
* ps3_context *ctx = ps3 device information
* u8 mode          = 0(file) or 1(hdd)
***********************************************************************/
s32 get_device_handle(ps3_context *ctx, u8 mode)
{
	s32 i, ret = -1;
	char drive[32];
	
	if(mode) {
		for(i = 1; i < 16; i++) {
			sprintf(drive, "\\\\.\\PhysicalDrive%d", i);
			ctx->dev = CreateFile(drive, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
			
			if(ctx->dev == INVALID_HANDLE_VALUE)
			  CloseHandle(ctx->dev);
		  else
		    if((ret = check_device(ctx)) == 0)
		      return 0;
		}
	}
	else {
		ctx->dev = CreateFile("backup.bin", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	  
	  if(ctx->dev == INVALID_HANDLE_VALUE)
		  CloseHandle(ctx->dev);
		
		if((ret = check_device(ctx)) == 0)
		  return 0;
	}
	
	return -1;
}

/***********************************************************************
* Read decrypted sector/s from ps3 hdd/image.
* 
* ps3_context *ctx = ps3 device information
* uint8_t *buf     = buffer to hold data
* s64 n_sec        = count of sectors to read
* s64 sec_num      = start sector number on hdd
***********************************************************************/
s64 block_read(ps3_context *ctx, u8 *buf, s64 n_sec, s64 sec_num)
{
	s64 i;
	DWORD n_read;
	
	// check device handle
	if(ctx->dev <= 0)
		return -1;
	
	// read and byte swap data
	seek_device(ctx->dev, sec_num * SECTOR_SIZE);
	ReadFile(ctx->dev, buf, n_sec * SECTOR_SIZE, &n_read, 0);
	_es16_buffer(buf, n_sec * SECTOR_SIZE);
	
	// decrypt layer 1(ATA)
  switch(ctx->ps3_type) {
    case 1:  // FAT_NAND
	  case 2:  // FAT_NOR
	    for(i = 0; i < n_sec; i++) {
		    aes_crypt_cbc(&cbc_dec, AES_DECRYPT, SECTOR_SIZE, ctx->iv, buf + (SECTOR_SIZE * i), buf + (SECTOR_SIZE * i));
		    memset(ctx->iv, 0, 16);
      }
	    break;
	  case 3:  // SLIM_NOR
		  for(i = 0; i < n_sec; i++)
		    aes_xts_crypt(&xts_dec, sec_num + i, SECTOR_SIZE, buf + (SECTOR_SIZE * i), buf + (SECTOR_SIZE * i));
		  break;
  }
	
	// if data into VFLASH region, decrypt layer 2 too
	if(ctx->vflash_start != 0)
		for(i = 0; i < n_sec; i++)
			if(((sec_num + i) >= ctx->vflash_start) && ((sec_num + i) <= (ctx->vflash_start + ctx->vflash_size)))
			  aes_xts_crypt(&xts_dec_vf, sec_num + i, SECTOR_SIZE, buf + (SECTOR_SIZE * i), buf + (SECTOR_SIZE * i));
  
	return (s64)n_read;
}

/***********************************************************************
* Write encrypted sector/s to ps3 hdd/image.
* 
* ps3_context *ctx = ps3 device information
* uint8_t *buf     = buffer with data to write
* s64 n_sec        = count of sectors to write
* s64 sec_num      = start sector number on hdd
***********************************************************************/
s64 block_write(ps3_context *ctx, u8 *buf, s64 n_sec, s64 sec_num)
{
	s64 i;
	DWORD n_write;
  	
	// check device handle
	if(ctx->dev <= 0)
		return -1;
	
	// if data into VFLASH region, encrypt layer 2(VFLASH) first
	if(ctx->vflash_start != 0)
		for(i = 0; i < n_sec; i++)
			if(((sec_num + i) >= ctx->vflash_start) && ((sec_num + i) <= (ctx->vflash_start + ctx->vflash_size)))
			  aes_xts_crypt(&xts_enc_vf, sec_num + i, SECTOR_SIZE, buf + (SECTOR_SIZE * i), buf + (SECTOR_SIZE * i));
	
	// encrypt layer 1(ATA)
  switch(ctx->ps3_type) {
    case 1:  // FAT_NAND
	  case 2:  // FAT_NOR
	    for(i = 0; i < n_sec; i++) {
		    aes_crypt_cbc(&cbc_enc, AES_ENCRYPT, SECTOR_SIZE, ctx->iv, buf + (SECTOR_SIZE * i), buf + (SECTOR_SIZE * i));
		    memset(ctx->iv, 0, 16);
      }
	    break;
	  case 3:  // SLIM_NOR
		  for(i = 0; i < n_sec; i++)
		    aes_xts_crypt(&xts_enc, sec_num + i, SECTOR_SIZE, buf + (SECTOR_SIZE * i), buf + (SECTOR_SIZE * i));
		  break;
  }
	
	// byte swap data and write to HDD
	_es16_buffer(buf, n_sec * SECTOR_SIZE);
	seek_device(ctx->dev, sec_num * SECTOR_SIZE);
	WriteFile(ctx->dev, buf, n_sec * SECTOR_SIZE, &n_write, NULL);
	
	return (s64)n_write;
}

/***********************************************************************
* Read decrypted data from ps3 hdd/file.
* 
* ps3_context *ctx = ps3 device information
* uint8_t *buf     = buffer to hold data
* int64_t numbytes = read size in bytes(must not be sector aligned)
* int64_t dev_off	 = byte offset on hdd(must not be sector aligned)
***********************************************************************/
s64 device_read(ps3_context *ctx, u8 *buf, u64 numbytes, s64 dev_off)
{
	s64 r_sec = 0, r_byte = 0, remain = 0;
	u8 tmp1[SECTOR_SIZE];
	
	u64 seq_nr    = dev_off / SECTOR_SIZE;
	u64 byte_rest = dev_off % SECTOR_SIZE;
	u64 sec_rest  = SECTOR_SIZE - byte_rest;
	
  block_read(ctx, tmp1, 1, seq_nr);
  
	if(numbytes <= sec_rest) {
		memcpy(buf, tmp1 + byte_rest, numbytes);
		return numbytes;
	}
	
	if(numbytes > sec_rest) {
		memcpy(buf, tmp1 + byte_rest, sec_rest);
		
		remain = numbytes - sec_rest;
		
		if((remain / SECTOR_SIZE) > 0) {
			r_sec = remain / SECTOR_SIZE;
			block_read(ctx, (buf + sec_rest), r_sec, (seq_nr + 1));
			
			if((numbytes - sec_rest - (r_sec * SECTOR_SIZE)) > 0) {
				r_byte = numbytes - sec_rest - (r_sec * SECTOR_SIZE);
				block_read(ctx, tmp1, 1, (seq_nr + 1 + r_sec));
				memcpy(buf + sec_rest + (r_sec * SECTOR_SIZE), tmp1, r_byte);
				return numbytes;
			}
			return numbytes;
		}
		
		r_byte = numbytes - sec_rest;
		block_read(ctx, tmp1, 1, (seq_nr + 1));
		memcpy(buf + sec_rest, tmp1, r_byte);
		return numbytes;
	}
	
	return 0;
}

/***********************************************************************
* Write encrypted data to ps3 hdd.
* 
* ps3_context *ctx = ps3 device information
* uint8_t *buf     = buffer with data to encrypt and write
* int64_t numbytes = write size in bytes(must not be sector aligned)
* int64_t dev_off	 = byte offset on hdd(must not be sector aligned)
***********************************************************************/
s64 device_write(ps3_context *ctx, u8 *buf, u64 numbytes, s64 dev_off)
{
	s64 r_sec = 0, r_byte = 0, remain = 0;
	u8 tmp1[SECTOR_SIZE];
	
	u64 seq_nr    = dev_off / SECTOR_SIZE;
	u64 byte_rest = dev_off % SECTOR_SIZE;
	u64 sec_rest  = SECTOR_SIZE - byte_rest;
	
  block_read(ctx, tmp1, 1, seq_nr);
  
  if(numbytes <= sec_rest) {
		memcpy(tmp1 + byte_rest, buf, numbytes);
		block_write(ctx, tmp1, 1, seq_nr);
		return numbytes;
	}
  
	if(numbytes > sec_rest) {
		memcpy(tmp1 + byte_rest, buf, sec_rest);
		block_write(ctx, tmp1, 1, seq_nr);
		
		remain = numbytes - sec_rest;
		
		if((remain / SECTOR_SIZE) > 0) {
			r_sec = remain / SECTOR_SIZE;
			block_write(ctx, buf + sec_rest, r_sec, seq_nr + 1);
			
			if((numbytes - sec_rest - (r_sec * SECTOR_SIZE)) > 0) {
				r_byte = numbytes - sec_rest - (r_sec * SECTOR_SIZE);
				block_read(ctx, tmp1, 1, (seq_nr + 1 + r_sec));
				memcpy(tmp1, buf + sec_rest + (r_sec * SECTOR_SIZE), r_byte);
				block_write(ctx, tmp1, 1, (seq_nr + 1 + r_sec));
				return numbytes;
			}
			return numbytes;
		}
		
		r_byte = numbytes - sec_rest;
		block_read(ctx, tmp1, 1, (seq_nr + 1));
		memcpy(tmp1, buf + sec_rest, r_byte);
		block_write(ctx, tmp1, 1, (seq_nr + 1));
		return numbytes;
	}
	
	return 0;
}

/***********************************************************************
* Get all partitions on a ps3 hdd/image.
* 
* ps3_context *ctx = ps3 device information
***********************************************************************/
s32 get_partitions(ps3_context *ctx)
{
	s64 i, start;
	struct disklabel *pt;
	u8 *buf = malloc(8 * SECTOR_SIZE);
	u8 *tmp = malloc(144 * SECTOR_SIZE);
	
	// read ps3 hdd partition table
	device_read(ctx, buf, (8 * SECTOR_SIZE), (0 * SECTOR_SIZE));
	
	pt = (struct disklabel *)buf;
	
	for(i = 0; i < MAX_PARTITIONS; i++) {
		if(_ES64(pt->d_partitions[i].p_start) != 0) {
			
		  // get the first 144 sectors from partition start for fs test
		  start = _ES64(pt->d_partitions[i].p_start);
		  device_read(ctx, tmp, (144 * SECTOR_SIZE), (start * SECTOR_SIZE));
		  
		  check_partition_vflash(ctx, tmp);
		  check_partition_gameOS(ctx, tmp, _ES64(pt->d_partitions[i].p_start), _ES64(pt->d_partitions[i].p_size));
		  check_partition_gameOS_swap(ctx, tmp, _ES64(pt->d_partitions[i].p_start), _ES64(pt->d_partitions[i].p_size));
		}
	}
	
	free(buf);
	free(tmp);
	return 0;
}
