/*
* Copyright (c) 2019 by 3141card
* This file is released under the GPLv2.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>

#include "types.h"
#include "kgen.h"
#include "device.h"
#include "util.h"
#include "fs/ufs.h"
#include "fs/fat.h"





/***********************************************************************
* Read sector/s from HDD.
* 
* ps3_context *ctx = ps3 device information
* const char *name = output file name
* s64 start        = start sector on hdd
* s64 count        = count of sectors to read
***********************************************************************/
void read_sector(ps3_context *ctx, const char *name, s64 start, s64 count)
{
	s64 i;
	u8 *buf = NULL;
	FILE *fd = fopen(name, "wb");
	s64 n_64k = count / 128;
	s64 n_sec = count % 128;
	
	if(n_64k) {
		buf = malloc(128 * SECTOR_SIZE);
		for(i = 0; i < n_64k; i++) {
		  block_read(ctx, buf, 128, start);
		  fwrite(buf, sizeof(u8), (128 * SECTOR_SIZE), fd);
		  start += 128;
		}
		free(buf);
  }
  
  if(n_sec) {
		buf = malloc(n_sec * SECTOR_SIZE);
		block_read(ctx, buf, n_sec, start);
		fwrite(buf, sizeof(u8), (n_sec * SECTOR_SIZE), fd);
		free(buf);
	}
	
	fclose(fd);
}

/***********************************************************************
* Write sector/s to HDD.
* 
* ps3_context *ctx = ps3 device information
* const char *name = input file name
* s64 start        = start sector on hdd
* s64 count        = count of sectors to write
***********************************************************************/
void write_sector(ps3_context *ctx, const char *name, s64 start, s64 count)
{
	s64 i;
	u8 *buf = NULL;
	FILE *fd = fopen(name, "rb");
	s32 file_size = 0;
	s64 n_64k = count / 128;
	s64 n_sec = count % 128;
	
  fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if(file_size != (count * SECTOR_SIZE)) {
		printf("File size not equal size to write!\n");
	  return;
	}
	
	if(n_64k) {
		buf = malloc(128 * SECTOR_SIZE);
		for(i = 0; i < n_64k; i++) {
			fread(buf, sizeof(u8), (128 * SECTOR_SIZE), fd);
			block_write(ctx, buf, 128, start);
		  start += 128;
		}
		free(buf);
  }
  
  if(n_sec) {
		buf = malloc(n_sec * SECTOR_SIZE);
	  fread(buf, sizeof(u8), (n_sec * SECTOR_SIZE), fd);
	  block_write(ctx, buf, n_sec, start);
		free(buf);
	}
	
	fclose(fd);
}









/***********************************************************************
* main
***********************************************************************/
s32 main(s32 argc, char *argv[])
{
	s32 ret = -1;
	u8 *eid_root_key = {0};							// eid root key/iv
	ps3_context *ctx = NULL;            // ps3 hdd context
	struct fs *ufs2;										// superblock(UFS2)
	struct fat_bs *fat_fs;              // bootsector(FAT12/16)
	struct fat32_bs *fat32;           	// bootsector(FAT32)
	ufs_inop root_ino = ROOTINO;
	ufs_inop ino;
	
	
	// init ps3 context
	ctx = malloc(sizeof(ps3_context));
	memset(ctx, 0, sizeof(ps3_context));
	
	// load rootkey from file
	if((eid_root_key = _read_buffer((s8*)"eid_root_key", NULL)) == NULL) {		
		printf("file \"eid_root_key\" not found !\n");
		goto end;
	}
	
	// generate ata/encdec keys
	generate_ata_keys(eid_root_key, eid_root_key + 0x20, ctx->ata_k1, ctx->ata_k2);		
  generate_encdec_keys(eid_root_key, eid_root_key + 0x20, ctx->encdec_k1, ctx->encdec_k2);
	free(eid_root_key);
	
	if(argc <= 1) {
		goto end;
	}
	
	// get hdd handle and ps3 type
	if(strcmp(argv[1], "hdd") == 0){
    if((ret = get_device_handle(ctx, 1)) == -1) {
		  printf("no PS3 hdd found or eid root key wrong!\n");
		  goto end;
	  }
  }
  
  if(strcmp(argv[1], "file") == 0){
    if((ret = get_device_handle(ctx, 0)) == -1) {
      printf("file \"backup.bin\" not found!\n");
		  goto end;
	  }
  }
	
	// get all partitions
	get_partitions(ctx);
	
	// list available volumes 
	if(argc == 2) {
	  printf("\navailable volumes are...\n\n");
	  if(ctx->hdd0_start != 0)
	    printf(" dev_hdd0\n");
    if(ctx->hdd1_start != 0)
      printf(" dev_hdd1\n");
    if(ctx->flash_start != 0)
      printf(" dev_flash\n");
    if(ctx->flash2_start != 0)
      printf(" dev_flash2\n");
    if(ctx->flash3_start != 0)
      printf(" dev_flash3\n");
    else printf("no volumes available.\n");
	}
	
	if(argc == 3) {
		goto end;
	}
	
	// DEBUG: print sector by hdd byte offset
	if(argc == 4) {
		if(strcmp(argv[2], "print") == 0) {
			u8 *buf = malloc(SECTOR_SIZE);
			s64 sec_num = strtoll(argv[3], NULL, 16);
			block_read(ctx, buf, 1, sec_num);
			_print_buf(buf, 0, 32);
			free(buf);
		}
		goto end;
	}
	
	if(argc == 5) {
		if(strncmp(argv[2], "dev_", 4) == 0) {
		  if(strcmp(argv[2], "dev_hdd0") == 0) {
			  ufs2 = ufs_init(ctx);																								       // init gameOS...
			  if(ufs2 == NULL) {
			  	printf("can't open dev_hdd0!\n");
			  }
			  if(strcmp(argv[3], "dir") == 0 || strcmp(argv[3], "ls") == 0) {    		     // show dir...
			  	ufs_print_dir_list(ctx, ufs2, (u8*)argv[4], (u8*)argv[2]);    
			  	free(ufs2);
		  	}
		  	else if(strcmp(argv[3], "copy") == 0 || strcmp(argv[3], "cp") == 0) {	     // copy file/dir...
		  		ino = ufs_lookup_path(ctx, ufs2, (u8*)argv[4], 0, ROOTINO);
		  		ufs_copy_data(ctx, ufs2, root_ino, ino, argv[4], 0);
		  		free(ufs2);
		  	}
		  	else if(strcmp(argv[3], "replace") == 0) {                                 // replace file
			    ino = ufs_lookup_path(ctx, ufs2, (u8*)argv[4], 0, ROOTINO);
			    ufs_replace_data(ctx, ufs2, root_ino, ino, argv[4]);
			    free(ufs2);
		    }
			  else{
			  	free(ufs2);
		  	}
		  }
			// if partition dev_hdd1(FAT32)...
		  else if(strcmp(argv[2], "dev_hdd1") == 0) {
		  	fat32 = init_fat32(ctx, ctx->hdd1_start);
		  	if(fat32 == NULL) {
			  	printf("can't open dev_hdd1!\n");
		  	}
		  	ctx->hdd1_free = fat_how_many_free_bytes(ctx, ctx->hdd1_start, NULL, fat32);
		  	if(strcmp(argv[3], "dir") == 0 || strcmp(argv[3], "ls") == 0) {				  			// show dir...
				  fat_print_dir_list(ctx, ctx->hdd1_start, NULL, fat32, (u8*)argv[4], (u8*)argv[2], ctx->hdd1_free);
				  free(fat32);
			  }
			  else if(strcmp(argv[3], "copy") == 0 || strcmp(argv[3], "cp") == 0) {	  			// copy file/dir...
			  	fat_copy_data(ctx, ctx->hdd1_start, NULL, fat32, argv[4], 0);
			  	free(fat32);
			  }
			  else if(strcmp(argv[3], "replace") == 0) {                                 // replace file
			    fat_replace_data(ctx, ctx->hdd1_start, NULL, fat32, argv[4]);
			    free(fat32);
		    }
			  else {
		  		free(fat32);
		  	}
		  }
			// if partition dev_flash(FAT16)...
		  else if(strcmp(argv[2], "dev_flash") == 0) {	
		  	fat_fs = init_fat_old(ctx, ctx->flash_start);
		  	if(fat_fs == NULL) {
		  		printf("can't open dev_flash!\n");
			  }	
			  ctx->flash_free = fat_how_many_free_bytes(ctx, ctx->flash_start, fat_fs, NULL);
			  if(strcmp(argv[3], "dir") == 0 || strcmp(argv[3], "ls") == 0) {
			  	fat_print_dir_list(ctx, ctx->flash_start, fat_fs, NULL, (u8*)argv[4], (u8*)argv[2], ctx->flash_free);
		  		free(fat_fs);	
			  }
			  else if(strcmp(argv[3], "copy") == 0 || strcmp(argv[3], "cp") == 0) {
			  	fat_copy_data(ctx, ctx->flash_start, fat_fs, 0, argv[4], 0);
			  	free(fat_fs);
			  }
			  else if(strcmp(argv[3], "replace") == 0) {                                 // replace file
			    fat_replace_data(ctx, ctx->flash_start, fat_fs, NULL, argv[4]);
			    free(fat_fs);
		    }
			  else {
			  	free(fat_fs);
			  }
		  }
			// if partition dev_flash2(FAT16)...
		  else if(strcmp(argv[2], "dev_flash2") == 0) {
		  	fat_fs = init_fat_old(ctx, ctx->flash2_start);
		  	if(fat_fs == NULL) {
		  		printf("can't open dev_flash2!\n");
		  	}
		  	ctx->flash2_free = fat_how_many_free_bytes(ctx, ctx->flash2_start, fat_fs, NULL);
		  	if(strcmp(argv[3], "dir") == 0 || strcmp(argv[3], "ls") == 0) {
		  		fat_print_dir_list(ctx, ctx->flash2_start, fat_fs, NULL, (u8*)argv[4], (u8*)argv[2], ctx->flash2_free);
		  		free(fat_fs);
		  	}
			  else if(strcmp(argv[3], "copy") == 0 || strcmp(argv[3], "cp") == 0) {
			  	fat_copy_data(ctx, ctx->flash2_start, fat_fs, 0, argv[4], 0);
		    	free(fat_fs);
		  	}
		  	else if(strcmp(argv[3], "replace") == 0) {                                 // replace file
			    fat_replace_data(ctx, ctx->flash2_start, fat_fs, NULL, argv[4]);
			    free(fat_fs);
		    }
		  	else {
		  		free(fat_fs);
		  	}
		  }
			// if partition dev_flash3(FAT12)...
		  else if(strcmp(argv[2], "dev_flash3") == 0) {
	    	fat_fs = init_fat_old(ctx, ctx->flash3_start);
		  	if(fat_fs == NULL) {
		  		printf("can't open dev_flash3!\n");
		  	}
		  	ctx->flash3_free = fat_how_many_free_bytes(ctx, ctx->flash3_start, fat_fs, NULL);
			  if(strcmp(argv[3], "dir") == 0 || strcmp(argv[3], "ls") == 0) {
			  	fat_print_dir_list(ctx, ctx->flash3_start, fat_fs, NULL, (u8*)argv[4], (u8*)argv[2], ctx->flash3_free);
			  	free(fat_fs);	
			  }
		  	else if(strcmp(argv[3], "copy") == 0 || strcmp(argv[3], "cp") == 0) {
			  	fat_copy_data(ctx, ctx->flash3_start, fat_fs, 0, argv[4], 0);
			  	free(fat_fs);	
			  }
			  else if(strcmp(argv[3], "replace") == 0) {                                 // replace file
			    fat_replace_data(ctx, ctx->flash3_start, fat_fs, NULL, argv[4]);
			    free(fat_fs);
		    }
			  else {
			  	free(fat_fs);	
			  }
		  }
		  else {
		  	printf("no such volume!\n");
		  }
		}
	}
	
	// read/write sector/s from/to HDD
	if(argc == 6) {
		if(strcmp(argv[2], "read_block") == 0) {
			read_sector(ctx, argv[3], strtoll(argv[4], NULL, 16), strtoll(argv[5], NULL, 16));
		}
		if(strcmp(argv[2], "write_block") == 0) {
			write_sector(ctx, argv[3], strtoll(argv[4], NULL, 16), strtoll(argv[5], NULL, 16));
		}
	}
	
end:
  if(ctx) free(ctx);
	
	return 0;
}






