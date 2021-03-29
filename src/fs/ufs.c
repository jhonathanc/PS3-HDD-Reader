/*
* Copyright (c) 2019 by 3141card
* This file is released under the GPLv2.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>

#include "ufs.h"


/***********************************************************************
*Funktion: read_direntry, liest einen directory-entry ein
* 	void *buf 						= pointer auf puffer mit dir-entrie daten
* 	struct direct* direct = pointer auf struct für dir-entries
* return:
* 	dir->d_reclen         = länge des verarbeiteten record's aus buf
***********************************************************************/
static s32 ufs_read_direntry(void *buf, struct direct* dir)
{
	memcpy(dir, buf, 8); 
	strncpy(dir->d_name, &((char*)buf)[8], dir->d_namlen); 
	dir->d_name[dir->d_namlen] = '\0'; 
	return _ES16(dir->d_reclen); 
}
/***********************************************************************
*Funktion zum freigeben einer blockliste.
***********************************************************************/
static void ufs_free_block_list(ufs2_block_list *list)
{
	free(list->blk_add);
	free(list);
}
/***********************************************************************
*sortier function für directory entries
* 
***********************************************************************/
static s32 ufs_sort_dir(const void *first, const void *second)
{
	const struct direct *a = first;
	const struct direct *b = second;

	if(!strcmp(a->d_name, ".")){
		return -1;
	}else if(!strcmp(b->d_name, ".")){
		return 1;
	}else if(!strcmp(a->d_name, "..")){
		return -1;
	}else if(!strcmp(b->d_name, "..")){
		return 1;
	}else{
		return strcmp(a->d_name, b->d_name);
	}
}
/***********************************************************************
*Funktion zum einlesen von datenblöcken in einen puffer, basierend auf
*einer blockliste.
* 	s32 device								  = device-handle
* 	struct fs *fs								= superblock des filesystems
* 	struct ufs2_dinode *di			= 
* 	ufs2_block_list *block_list	= blocklist
* 	u8 *buf											= buffer zum befüllen
* 	ufs_inop start_block				= 
* 	ufs_inop num_blocks					= 
***********************************************************************/
static s32 ufs_read_data_by_blocklist(ps3_context *ctx, struct fs *ufs2, struct ufs2_dinode *di, ufs2_block_list *block_list,
		u8 *buf, ufs_inop start_block, ufs_inop num_blocks)
{
	s32 i, bsize;
	s64 total, read, len, offset;
	s64 *bl;
	
	bl = block_list->blk_add;

	if(!num_blocks){
		len = _ES64(di->di_size);
	}
	else{
		len = num_blocks * ufs2->fs_fsize;
	}
	
	if(_ES64(di->di_blocks) == 0){
		memcpy(buf, di->di_db, _ES64(di->di_size));
		return _ES64(di->di_size);
	}

	total  = 0;
	offset = 0;
	
	for(i = 0; total < start_block * ufs2->fs_fsize; ++i){
		bsize = sblksize(ufs2, _ES64(di->di_size), i);

		if(total + bsize > start_block * ufs2->fs_fsize){
			offset = start_block * ufs2->fs_fsize - total;
			break;
		}
		total += bsize;    
	}
	
	for(total = 0; total < len; ++i){
		if(bl[i] != 0 && seek_device(ctx->dev, (bl[i] * ufs2->fs_fsize) + (ctx->hdd0_start * SECTOR_SIZE) + (offset * SECTOR_SIZE)) <= 0)
			return -1;
		
		read = sblksize(ufs2, _ES64(di->di_size), i) - offset;
		
		offset = 0;
		
		if(read + total > len)
			read = len - total;
		
		if(bl[i] == 0){
			memset(buf, 0, read);
		}
		else if(device_read(ctx, buf, read, ((bl[i] * ufs2->fs_fsize) + (ctx->hdd0_start * SECTOR_SIZE) + offset)) < 0) {
			return -1;
		}

		total += read;
		buf += read;
	}

	return total;
}
/***********************************************************************
*Funktion zum einlesen eines inode in eine ufs2_dinode struct.
*		s32 device					   = device-handle
* 	struct fs *fs  				 = superblock des filesystems
* 	ufs_inop ino					 = zu lesender inode
* 	struct ufs2_dinode *di = pointer auf zu fühlende struct
***********************************************************************/
static s32 read_inode(ps3_context *ctx, struct fs *ufs2, ufs_inop ino, struct ufs2_dinode *di)
{
	s64 byte_offset; 
	s64 cg_nmb = ino / ufs2->fs_ipg; 
	u8 *buf = malloc(sizeof(struct ufs2_dinode)); 
  
	byte_offset = (ctx->hdd0_start * SECTOR_SIZE) + 
							  (ufs2->fs_iblkno * ufs2->fs_fsize) + 
							  (cg_nmb * (ufs2->fs_fpg * ufs2->fs_fsize)) +  
							  ((ino % ufs2->fs_ipg) * sizeof(struct ufs2_dinode)); 
	
	device_read(ctx, buf, sizeof(struct ufs2_dinode), byte_offset); 
	
	memcpy((u8*)di, buf, sizeof(*di)); 
	
	return 0;
}
/***********************************************************************
*Funktion vom typ ufs2_block_list, zum erstellen der blockliste für
*einen inode.
* 	s32 device					  	= device-handle
* 	struct fs *fs						= superblock des filesystems
* 	struct ufs2_dinode *di	= pointer auf struct des inodes von dem
* 														blocklist erstellt werden soll
* return: ufs2_block_list*
***********************************************************************/
static ufs2_block_list* get_block_list(ps3_context *ctx, struct fs *ufs2, struct ufs2_dinode *di)
{ 
	s32 i, j, k;
	s64 count, totalsize;
	s64 *block_list;
	ufs2_block_list *list;
	s64 *bufx;
	s64 *bufy;
	s64 *bufz;
	
	
	block_list = malloc(((_ES64(di->di_size) / ufs2->fs_bsize) + 1) * sizeof(*block_list));
	list = malloc(sizeof(*list));
	list->blk_add = block_list;
	
	count = 0;
	totalsize = 0;
	
	for(i = 0; i < NDADDR; ++i){
		block_list[count] = _ES64(di->di_db[i]);
		totalsize += sblksize(ufs2, _ES64(di->di_size), count);
		++count;

		if(totalsize >= _ES64(di->di_size)){
			return list;
		}	
	}
	
	bufx = malloc(ufs2->fs_bsize);
	device_read(ctx, (u8*)bufx, ufs2->fs_bsize, (ctx->hdd0_start * SECTOR_SIZE) + (_ES64(di->di_ib[0]) * ufs2->fs_fsize));
	
	for(i = 0; i * sizeof(bufx[0]) < ufs2->fs_bsize; ++i){
		block_list[count] = _ES64(bufx[i]);
		totalsize += sblksize(ufs2, _ES64(di->di_size), count);
		++count;
		if(totalsize >= _ES64(di->di_size)){
			free(bufx);
			return list;
		}
	}
  
	bufy = malloc(ufs2->fs_bsize);
	device_read(ctx, (u8*)bufy, ufs2->fs_bsize, (ctx->hdd0_start * SECTOR_SIZE) + (_ES64(di->di_ib[1]) * ufs2->fs_fsize));
	
	for(j = 0; j * sizeof(bufy[0]) < ufs2->fs_bsize; ++j) {
		device_read(ctx, (u8*)bufx, ufs2->fs_bsize, (ctx->hdd0_start * SECTOR_SIZE) + (_ES64(bufy[j]) * ufs2->fs_fsize));
		
		for(i = 0; i * sizeof(bufx[0]) < ufs2->fs_bsize; ++i){
			block_list[count] = _ES64(bufx[i]);
			totalsize += sblksize(ufs2, _ES64(di->di_size), count);
			++count;
			
			if(totalsize >= _ES64(di->di_size)){
				free(bufx);
				free(bufy);
				return list;
			}
		}
	}
  
	bufz = malloc(ufs2->fs_bsize);
	device_read(ctx, (u8*)bufz, ufs2->fs_bsize, (ctx->hdd0_start * SECTOR_SIZE) + (_ES64(di->di_ib[2]) * ufs2->fs_fsize));

	for(k = 0; k * sizeof(bufz[0]) < ufs2->fs_bsize; ++k){
		device_read(ctx, (u8*)bufy, ufs2->fs_bsize, (ctx->hdd0_start * SECTOR_SIZE) + (_ES64(bufz[k]) * ufs2->fs_fsize));

		for(j = 0; j * sizeof(bufy[0]) < ufs2->fs_bsize; ++j){
			device_read(ctx, (u8*)bufx, ufs2->fs_bsize, (ctx->hdd0_start * SECTOR_SIZE) + (_ES64(bufz[j]) * ufs2->fs_fsize));

			for(i = 0; i * sizeof(bufx[0]) < ufs2->fs_bsize; ++i){
				block_list[count] = _ES64(bufx[i]);
				totalsize += sblksize(ufs2, _ES64(di->di_size), count);
				++count;

				if (totalsize >= _ES64(di->di_size)) {
					free(bufx);
					free(bufy);
					free(bufz);
					return list;
				}
			}
		}
	}

	free(bufx);
	free(bufy);
	free(bufz);
	return list;
}
/***********************************************************************
*Funktion: 
* 	s32 device        = device-handle
* 	struct fs *fs     = superblock, struct fs
* 	ufs_inop root_ino = 
* 	ufs_inop ino      = 
***********************************************************************/
static ufs_inop ufs_follow_symlinks(ps3_context *ctx, struct fs* ufs2, ufs_inop root_ino, ufs_inop ino)
{
	struct ufs2_dinode dino;
	ufs2_block_list *block_list;
	
	read_inode(ctx, ufs2, ino, &dino);
	
	while((_ES16(dino.di_mode) & IFMT) == IFLNK){ 
		u8 tmpname[MAX_PATH];
		block_list = get_block_list(ctx, ufs2, &dino);
		ufs_read_data_by_blocklist(ctx, ufs2, &dino, block_list, tmpname, 0, 0);
		ufs_free_block_list(block_list);
		tmpname[_ES64(dino.di_size)] = '\0';
		ino = ufs_lookup_path(ctx, ufs2, tmpname, 0, root_ino);
		read_inode(ctx, ufs2, ino, &dino);
	}

	return ino; 
}
/***********************************************************************
* init ufs2, read superblock.
* 
***********************************************************************/
struct fs* ufs_init(ps3_context *ctx)
{ 
	struct fs *fs;
	u8 *buf = malloc(SBLOCKSIZE);
	
	device_read(ctx, buf, SBLOCKSIZE, (ctx->hdd0_start + 128) * SECTOR_SIZE);
	
	fs = (struct fs*)buf;
	
	fs->fs_sblkno						= _ES32(fs->fs_sblkno);
	fs->fs_cblkno						= _ES32(fs->fs_cblkno);
	fs->fs_iblkno						= _ES32(fs->fs_iblkno);
	fs->fs_dblkno						= _ES32(fs->fs_dblkno);
	fs->fs_ncg							= _ES32(fs->fs_ncg);
	fs->fs_bsize						= _ES32(fs->fs_bsize);
	fs->fs_fsize						= _ES32(fs->fs_fsize);
	fs->fs_frag							= _ES32(fs->fs_frag);
	fs->fs_minfree					= _ES32(fs->fs_minfree);
	fs->fs_bmask						= _ES32(fs->fs_bmask);
	fs->fs_fmask						= _ES32(fs->fs_fmask);
	fs->fs_bshift						= _ES32(fs->fs_bshift);
	fs->fs_fshift						= _ES32(fs->fs_fshift);
	fs->fs_maxcontig				= _ES32(fs->fs_maxcontig);
	fs->fs_maxbpg						= _ES32(fs->fs_maxbpg);
	fs->fs_fragshift				= _ES32(fs->fs_fragshift);
	fs->fs_fsbtodb					= _ES32(fs->fs_fsbtodb);
	fs->fs_sbsize						= _ES32(fs->fs_sbsize);
	fs->fs_nindir						= _ES32(fs->fs_nindir);
	fs->fs_inopb						= _ES32(fs->fs_inopb);
	fs->fs_old_nspf					= _ES32(fs->fs_old_nspf);
	fs->fs_cssize						= _ES32(fs->fs_cssize);
	fs->fs_cgsize						= _ES32(fs->fs_cgsize);
	fs->fs_spare2						= _ES32(fs->fs_spare2);
	fs->fs_ipg							= _ES32(fs->fs_ipg);
	fs->fs_fpg							= _ES32(fs->fs_fpg);
	fs->fs_pendingblocks		= _ES64(fs->fs_pendingblocks);
	fs->fs_pendinginodes		= _ES32(fs->fs_pendinginodes);
	fs->fs_avgfilesize			= _ES32(fs->fs_avgfilesize);
	fs->fs_avgfpdir					= _ES32(fs->fs_avgfpdir);
	fs->fs_save_cgsize			= _ES32(fs->fs_save_cgsize);
	fs->fs_flags						= _ES32(fs->fs_flags);
	fs->fs_contigsumsize		= _ES32(fs->fs_contigsumsize);
	fs->fs_maxsymlinklen		= _ES32(fs->fs_maxsymlinklen);
	fs->fs_old_inodefmt			= _ES32(fs->fs_old_inodefmt);
	fs->fs_maxfilesize			= _ES64(fs->fs_maxfilesize);
	fs->fs_qbmask						= _ES64(fs->fs_qbmask);
	fs->fs_qfmask						= _ES64(fs->fs_qfmask);
	fs->fs_state						= _ES32(fs->fs_state);
	fs->fs_old_postblformat = _ES32(fs->fs_old_postblformat);
	fs->fs_old_nrpos				= _ES32(fs->fs_old_nrpos);
	fs->fs_magic						= _ES32(fs->fs_magic);
	
	return fs;
}
/***********************************************************************
*Funktion: 
* 	s32 device        = device-handle
* 	struct fs *fs     = superblock
* 	u8 *path 		    	= pfad der verfolgt werden soll
* 	int follow        = 
* 	ufs_inop root_ino = 
***********************************************************************/
ufs_inop ufs_lookup_path(ps3_context *ctx, struct fs* ufs2, u8* path, int follow, ufs_inop root_ino)
{ 
	s32 ret, i; 
	u8 *s, *sorig, *nexts, *nexte;
	u8 *tmp;
	struct ufs2_dinode dino;
	struct direct dir;
	ufs2_block_list *block_list;
	s64 found_ino = 0;
	
	s = malloc(MAX_PATH);
	sorig = s;
	
	if(path[0] == '/'){ 
		memcpy(s, &path[1], strlen((char*)path)); 
		root_ino = ROOTINO; 
	}
	else{	 
		memcpy(s, path, strlen((char*)path));	 
	}
	
	for(nexts = s; nexts && nexts[0]; nexts = s){	 
		if((s = memchr(s, 0x2F, strlen((char*)s)))){						
			*s = '\0';
			s++;
		}
		
		read_inode(ctx, ufs2, root_ino, &dino); 
		block_list = get_block_list(ctx, ufs2, &dino); 
		tmp = malloc(_ES64(dino.di_size)); 
		ufs_read_data_by_blocklist(ctx, ufs2, &dino, block_list, tmp, 0, 0); 
		ufs_free_block_list(block_list); 
		
		nexte = tmp; 
		
		for(i = 0;; ++i){	 
			ret = ufs_read_direntry(nexte, &dir);	 
			
			if(nexte - tmp >= _ES64(dino.di_size)){	 
				free(tmp);
				return 0;
			}

			if(!strcmp(dir.d_name, (const char*)nexts)){ 
				found_ino = _ES32(dir.d_ino);	 
				break; 
			}
			nexte += ret; 
		}
		free(tmp); 
	
		if(!found_ino) 
			return 0;
	
		if(follow || (s && s[0] != '\0')){ 
			root_ino = ufs_follow_symlinks(ctx, ufs2, root_ino, found_ino);  
		}
		else{
			root_ino = found_ino;
		}
	}
	
	free(sorig);
	return root_ino;
}
/***********************************************************************
*Funktion: print_dir_list, gibt einen verzeichnis inhalt aus.
* 	s32 device    = device-handle
* 	struct fs *fs = struct fs
* 	u8 *path		  = pfad zum zu lesenden verzeichnis
***********************************************************************/
s32 ufs_print_dir_list(ps3_context *ctx, struct fs* ufs2, u8* path, u8* volume)
{
	s32 i, ret, entry_count;	
	u8 *buffer, *tmp;		
	u64 file_count, dir_count, byte_use, byte_free;
	ufs_inop inode;	
	struct ufs2_dinode *root_inode;	
	root_inode = malloc(sizeof(struct ufs2_dinode));	
	ufs2_block_list *block_list;	
	struct direct direntry_tmp;	
	struct direct *dirs;	
	struct ufs2_dinode dinode_tmp;	
	struct tm *tm;	
	char timestr[64];	
	char sizestr[32];	
	char byte_use_str[32];
	char byte_free_str[32];
	char symlinkstr[1028];		
	file_count = dir_count = byte_use = byte_free = 0;
	
	
	inode = ufs_lookup_path(ctx, ufs2, path, 1, ROOTINO); 
	
	if(!inode){ 
		printf("no such file or directory!\n");
		return -1;
	}
	
	read_inode(ctx, ufs2, inode, root_inode); 
	
	if (!(root_inode->di_mode & IFDIR)) { 
		printf("can't show, not a directory!\n");
		return -1;
	}
	 
	block_list = get_block_list(ctx, ufs2, root_inode);
	buffer = malloc(_ES64(root_inode->di_size) + sizeof(struct direct));
	ufs_read_data_by_blocklist(ctx, ufs2, root_inode, block_list, buffer, 0, 0);
	ufs_free_block_list(block_list);
	
	tmp = buffer;
  
	for(entry_count = 0; tmp - buffer < _ES64(root_inode->di_size); ++entry_count){
		ret = ufs_read_direntry(tmp, &direntry_tmp);
		tmp += ret;
	}
	
	dirs = malloc(entry_count * sizeof(*dirs));
	
	tmp = buffer;
	
	for(i = 0; i < entry_count; ++i){
		ret = ufs_read_direntry(tmp, &dirs[i]);
		tmp += ret;
	}
	
	free(buffer);
	
	qsort(dirs, entry_count, sizeof(*dirs), ufs_sort_dir);
  
	printf("\n Volume is ps3_hdd %s\n", volume);
	printf(" Directory of %s/%s\n\n", volume, path);
	
	for(i = 0; i < entry_count; ++i) {
		read_inode(ctx, ufs2, _ES32(dirs[i].d_ino), &dinode_tmp);
		
		dinode_tmp.di_mtime = _ES64(dinode_tmp.di_mtime);
		dinode_tmp.di_mode = _ES16(dinode_tmp.di_mode);
		
		tm = localtime((const time_t*) (&dinode_tmp.di_mtime));
		strftime(timestr, 64, "%m.%d.%Y  %H:%M", tm);
		
		// entry is link
		if ((dinode_tmp.di_mode & IFMT) == IFLNK) {
			u8 tmpname[1024];

			block_list = get_block_list(ctx, ufs2, &dinode_tmp);
			ufs_read_data_by_blocklist(ctx, ufs2, &dinode_tmp, block_list, tmpname, 0, 0);
			ufs_free_block_list(block_list);
			tmpname[dinode_tmp.di_size] = '\0';

			sprintf(symlinkstr, " -> %s", tmpname);
		}
		else{
			symlinkstr[0] = '\0';
		}
		
		// entry is dir
		if((dinode_tmp.di_mode & IFMT) == IFDIR){
			dir_count++;
			sprintf(sizestr, "<DIR>");
			printf("%s    %-14s %s\n", timestr, sizestr, dirs[i].d_name);
		}
		// entry is file
		else{
			file_count++;
			byte_use += _ES64(dinode_tmp.di_size);
			print_commas(_ES64(dinode_tmp.di_size), sizestr);
			printf("%s    %14s %s %s\n", timestr, sizestr, dirs[i].d_name, symlinkstr);
		}
	}
	
	print_commas(byte_use, byte_use_str);
	print_commas(_ES64(ufs2->fs_cstotal.cs_nbfree) * ufs2->fs_bsize, byte_free_str);
	printf("%16llu File(s),  ", file_count);
	printf("%s bytes\n", byte_use_str);
	printf("%16llu Dir(s),  ", dir_count);
	printf("%s bytes free\n", byte_free_str);
	
	free(dirs);
	return 0;
}
/***********************************************************************
*Funktion: read_file
* 	s32 device        = *hdd
* 	struct fs *fs     = struct fs
* 	ufs_inop root_ino = root
* 	ufs_inop ino      = 
* 	char *srcpath     = 
* 	char *destpath    = 
***********************************************************************/
s32 ufs_copy_data(ps3_context *ctx, struct fs *ufs2, ufs_inop root_ino, ufs_inop ino, char *srcpath, char *destpath)
{
  FILE *fd;
	const char *dir_ = "..";
	s64 i, totalsize, done, n_read;
	ufs2_block_list *block_list;
	char *buf, *dir, *tmp;
	char newdest[MAX_PATH];
	s32 using_con;
	struct ufs2_dinode dinode;
	struct utimbuf filetime;
	ufs_inop symlink_ino;
	s64 *bl;
	char string[MAX_PATH];
	
	
	if(strlen(srcpath) == 1 && srcpath[0] == 0x2F)
		return 0;
	
	using_con = (destpath && (!Stricmp(destpath, "CON") || !(Strnicmp(destpath, "CON.", 4))));
	
	if(!root_ino || !ino){
		printf("can't copy, no such file or directory!\n");
		return -1;
	}
	
	read_inode(ctx, ufs2, ino, &dinode);
	
	if((_ES16(dinode.di_mode) & IFMT) == IFLNK){  
		ino = ufs_lookup_path(ctx, ufs2, (u8*)srcpath, 1, ROOTINO);
		dir = dirname(srcpath);
		symlink_ino = ufs_lookup_path(ctx, ufs2, (u8*)dir, 1, ROOTINO);

		for(;; symlink_ino = ufs_lookup_path(ctx, ufs2, (u8*)dir_, 0, symlink_ino)){
			if(symlink_ino == ino){
				printf("\"%s\" is a recursive symlink loop\n", srcpath);
				free(dir);
				return -1;
			}

			if(symlink_ino == ROOTINO)
				break;
		}
		free(dir);
	}
	
	if(destpath == NULL){
		char *base = basename(srcpath);
		strcpy(newdest, "./");
		strcat(newdest, base);
		free(base);
	}
	else{	
		while(destpath[strlen(destpath) - 1] == '/')
			destpath[strlen(destpath) - 1] = '\0';
	
		strcpy(newdest, destpath);
	}
	
	read_inode(ctx, ufs2, ino, &dinode);
	block_list = get_block_list(ctx, ufs2, &dinode);
	totalsize = _ES64(dinode.di_size);
	
	if(_ES16(dinode.di_mode) & IFDIR){
		char *dirdest, *tmp;
		char nextsrc[256];
		char nextdest[MAX_PATH];
		int ret;
		struct direct direct_tmp;
		struct stat sb;

		if(using_con){
			printf("can't copy directory!\n");
			ufs_free_block_list(block_list);
			return -1;
		}

		if(!stat(newdest, &sb)){
			if((sb.st_mode & S_IFMT) != S_IFDIR){
				return -1;
			}
			dirdest = strdup(newdest);
		}
		else{
			if(mkdir(newdest, 0777) == -1){
				dirdest = valid_filename(newdest, 0);
				if(mkdir(dirdest,0777) == -1){
					free(dirdest);
					return -1;
				}
			}
			else{
				dirdest = strdup(newdest);
			}
		}
		
		buf = malloc(_ES64(dinode.di_size) + sizeof(struct direct));
		ufs_read_data_by_blocklist(ctx, ufs2, &dinode, block_list, (u8*)buf, 0, 0);
		tmp = buf;
		
		for(i = 0;; ++i){
			ret = ufs_read_direntry(tmp, &direct_tmp);
			
			if(tmp - buf >= _ES64(dinode.di_size) || !ret)
				break;
				
			tmp += ret;
			
			if(!strcmp(direct_tmp.d_name, ".") || !strcmp(direct_tmp.d_name, ".."))
				continue;

			strcpy(nextsrc, srcpath);
			
			if(srcpath[strlen(srcpath) - 1] != '/')
				strcat(nextsrc, "/");
				
			strcat(nextsrc, direct_tmp.d_name);
			strcpy(nextdest, dirdest);
			strcat(nextdest, "/");
			strcat(nextdest, direct_tmp.d_name);
			
			sprintf(string, "%s/%s", srcpath, direct_tmp.d_name);
			printf("copy -> %s\n", string);
			ufs_copy_data(ctx, ufs2, ino, _ES32(direct_tmp.d_ino), nextsrc, nextdest);
		}
		
		ufs_free_block_list(block_list);
		free(buf);

		return 0;
	}
	
	// entry is file...
	done = 0;
	n_read = 0;

	tmp = valid_filename(newdest, using_con);
	strcpy(newdest, tmp);
	free(tmp);
	
	fd = fopen(newdest, "wb");
	if(!fd) {
		printf("can't create file!\n");
		return -1;																			
	}
	
	buf = malloc(ufs2->fs_bsize);
	bl = block_list->blk_add;
	
	for(i = 0; done < totalsize; i++) {
		if(totalsize - done < ufs2->fs_bsize) { 
			n_read = device_read(ctx, (u8*)buf, totalsize - done, (bl[i] * ufs2->fs_fsize) + (ctx->hdd0_start * SECTOR_SIZE));
			
			fwrite(buf, 1, n_read, fd);
			done += n_read;
		}
		else { 
			n_read = device_read(ctx, (u8*)buf, ufs2->fs_bsize, (bl[i] * ufs2->fs_fsize) + (ctx->hdd0_start * SECTOR_SIZE));
			
			fwrite(buf, 1, n_read, fd);
			done += n_read;
		}
		fprintf(stderr,"(%03lld%%)\r", done * 100 / totalsize);
	}
	
	fclose(fd);
	filetime.actime = _ES64(dinode.di_atime);
	filetime.modtime = _ES64(dinode.di_mtime);
	utime(newdest, &filetime);
	
	free(buf);
	ufs_free_block_list(block_list);
	
	return 0;
}
/***********************************************************************
*Funktion: copy patched file back to hdd
* 	s32 device        = *hdd
* 	struct fs *fs     = struct fs
* 	ufs_inop root_ino = root
* 	ufs_inop ino      = 
* 	char *path        = 
***********************************************************************/
s32 ufs_replace_data(ps3_context *ctx, struct fs *ufs2, ufs_inop root_ino, ufs_inop ino, char *path)
{
	struct ufs2_dinode dinode;
	s64 i, size, totalsize, done = 0, n_write = 0;
	ufs2_block_list *block_list = NULL;
	u8 *buf_fd = NULL;
	FILE *fd = NULL;
	s64 *bl;
	
	
	if(strlen(path) == 1 && path[0] == 0x2F)
		return 0;
	
	if(!root_ino || !ino){
		printf("can't copy, no such file or directory!\n");
		return -1;
	}
	 
	if((fd = fopen(strrchr(path, '/') + 1, "rb")) == NULL) {
		printf("can't open patched file!\n");
		return 0;
	}
	
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	 
	read_inode(ctx, ufs2, ino, &dinode);
	
	if(_ES16(dinode.di_mode) & IFDIR) {
		printf("not a file!\n");
	  return 0;
	}
	totalsize = _ES64(dinode.di_size);
	
	if(size != totalsize) {
		printf("file size wrong!\n");
	  return 0;
	}
	 
	block_list = get_block_list(ctx, ufs2, &dinode);
	buf_fd = malloc(ufs2->fs_bsize);
	bl = block_list->blk_add;
	
	
	for(i = 0; done < totalsize; i++) {
		if(totalsize - done < ufs2->fs_bsize) {  
			fseek(fd, done, SEEK_SET);
			fread(buf_fd, sizeof(u8), totalsize - done, fd);
			n_write = device_write(ctx, buf_fd, totalsize - done, (bl[i] * ufs2->fs_fsize) + (ctx->hdd0_start * SECTOR_SIZE));
			done += n_write;
		}
		else {  
			fseek(fd, done, SEEK_SET);
			fread(buf_fd, sizeof(u8), ufs2->fs_bsize, fd);
			n_write = device_write(ctx, buf_fd, ufs2->fs_bsize, (bl[i] * ufs2->fs_fsize) + (ctx->hdd0_start * SECTOR_SIZE));
			done += n_write;
		}
		
		fprintf(stderr,"(%03lld%%)\r", done * 100 / totalsize);
	}
	
	free(buf_fd);
	fclose(fd);
	
	return 0;
}
