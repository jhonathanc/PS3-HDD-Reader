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

#include "fat.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
 
static time_t days_in_year[] = {
  0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 0, 0, 0,
};

/***********************************************************************
* 
***********************************************************************/
time_t fat2unix_time(u16 time, u16 date)
{
  time_t second, day, leap_day, month, year;
	
  year  = date >> 9;
  month = max(1, (date >> 5) & 0xf);
  day   = max(1, date & 0x1f) - 1;

  leap_day = (year + 3) / 4;
  
  if(year > YEAR_2100)
		leap_day--;
		
	if(IS_LEAP_YEAR(year) && month > 2)
		leap_day++;

	second =  (time & 0x1f) << 1;
	second += ((time >> 5) & 0x3f) * SECS_PER_MIN;
	second += (time >> 11) * SECS_PER_HOUR;
	second += (year * 365 + leap_day + days_in_year[month] + day + DAYS_DELTA) * SECS_PER_DAY;
		
	return second;
}
/***********************************************************************
* Init FAT12/16 file system.
* 
* ps3_context *ctx = ps3 device information
* u64 start        = 
***********************************************************************/
struct fat_bs* init_fat_old(ps3_context *ctx, u64 start)
{
	struct fat_bs* fat;
	u8 *buf = malloc(SECTOR_SIZE);
	device_read(ctx, buf, SECTOR_SIZE, (start * SECTOR_SIZE));
	fat = (struct fat_bs *)buf;
	return fat;
}
/***********************************************************************
* Init FAT32 file system.
* 
* ps3_context *ctx = ps3 device information
* u64 start        = 
***********************************************************************/
struct fat32_bs* init_fat32(ps3_context *ctx, u64 start)
{
	struct fat32_bs *fat32;
	u8 *buf = malloc(SECTOR_SIZE);
	device_read(ctx, buf, SECTOR_SIZE, (start * SECTOR_SIZE));
	fat32 = (struct fat32_bs *)buf;
	return fat32;
}
/***********************************************************************
* Get FAT file system type.
* 
* struct fat_bs *fat     = bootsector FAT12/16.
* struct fat32_bs *fat32 = bootsector FAT32.
* u8 type                = check for 1(FAT12/16) or 2(FAT32).
* 
* return: int (12, 16, 32)
***********************************************************************/
s32 get_fat_type(struct fat_bs *fat)
{
	s32 t1 = 0, t2 = 0;
	
	t1 = ((fat->bs_maxroot * 0x20) + (fat->bs_ssize - 1)) / fat->bs_ssize;
	
	if(fat->bs_tsec == 0)
		t2 = (fat->bs_nrsec - fat->bs_rsec - fat->bs_nrfat * fat->bs_fsize - t1) / fat->bs_csize;
	
	if(fat->bs_nrsec == 0)
		t2 = (fat->bs_tsec - fat->bs_rsec - fat->bs_nrfat * fat->bs_fsize - t1) / fat->bs_csize;
	
	if(t2 < 4085) return 12;                 // FAT12
	if(t2 >= 4085 && t2 < 65525) return 16;  // FAT16
	if(t1 == 0 && t2 >= 65525) return 32;    // FAT32
	
	return 0;
}
/***********************************************************************
* Return the size of the free memory in the file system.
* 
* ps3_context *ctx = ps3 device information
* u64 storage						 = partition
* struct fat_bs *fat_fs  = Fat12/16 bootsector
* struct fat32_bs *fat32 = Fat32 bootsector
***********************************************************************/
u64 fat_how_many_free_bytes(ps3_context *ctx, u64 storage, struct fat_bs *fat, struct fat32_bs *fat32)
{
	s32 i, t = 0;
	u64 count = 0, free_byte = 0, data_clu = 0;
	u8 *fat_buf, *buf;
	struct f_entry *add = NULL;
	
	
	if(fat) t = get_fat_type(fat);
		
	if(fat32) t = 32;
	
	switch(t) {
		case 12:
			data_clu = ((fat->bs_tsec + fat->bs_nrsec) * fat->bs_ssize - data_off(fat)) / clu_size(fat);/* data cluster in partition */
			buf = malloc(0x02);																																					/* buffer für entry allokieren... */
			fat_buf = malloc(fat_size(fat));																														/* buffer für FAT allokieren... */
			
			device_read(ctx, fat_buf, (fat_size(fat)), (storage * SECTOR_SIZE) + fat1_off(fat));
			
			for(i = 2; i < data_clu; i ++) {
				memcpy(buf, fat_buf + (fat12_e(fat, i) - fat1_off(fat)), 0x02);
				
				add = (struct f_entry*)buf;
				if((i % 2) == 0) {add->entry = add->entry & 0x00000FFF;}	/* entry adresse aufbereiten... */
				else {add->entry = add->entry >> 4 & 0x00000FFF;}
				add->entry &= 0x00000FFF;
					
				if(add->entry == 0x00000000)
					count++;
			}
			
			free_byte = count * clu_size(fat);
			free(add);
			free(fat_buf);
			return free_byte;
		break;
		case 16:
			data_clu = ((fat->bs_tsec + fat->bs_nrsec) * fat->bs_ssize - data_off(fat)) / clu_size(fat);	/* data cluster in partition */
			buf = malloc(0x02);																													/* buffer für entry allokieren... */
			fat_buf = malloc(fat_size(fat));																						/* buffer für FAT allokieren... */
      
			device_read(ctx, fat_buf, fat_size(fat), (storage * SECTOR_SIZE) + fat1_off(fat));
			
			for(i = 0; i < data_clu; i ++) {				  																  /* free cluster zählen... */
				memcpy(buf, fat_buf + (i * 2), 0x02);
				
				add = (struct f_entry*)buf;
				add->entry &= 0x0000FFFF;
						
				if(add->entry == 0x00000000)
					count++;
			}
			
			free_byte = count * clu_size(fat);
			
			free(add);
			free(fat_buf);
			return free_byte;
		break;
		case 32:
			data_clu = (fat32->bs_nrsec * fat32->bs_ssize - data_off32(fat32)) / clu_size32(fat32); /* data cluster in partition */
			buf = malloc(0x04);																																			/* buffer für entry allokieren... */
			fat_buf = malloc(fat_size32(fat32));							  																		/* buffer für FAT allokieren... */
      
			device_read(ctx, fat_buf, (fat_size32(fat32)), (storage * SECTOR_SIZE) + fat1_off32(fat32));
			
			for(i = 0; i < data_clu; i ++) {				  																		          /* free cluster zählen... */
				memcpy(buf, fat_buf + (i * 4), 0x04);
						
				add = (struct f_entry*)buf;
						
				if(add->entry == 0x00000000)
					count++;
			}
			
			free_byte = count * clu_size32(fat32);
			
			free(add);
			free(fat_buf);
			return free_byte;
		break;
	}
	
	return 0;
}
/***********************************************************************
* Get cluster count of a chain.
* 
* ps3_context *ctx       = ps3 device information
* u64 storage						 = partition
* struct fat_bs *fat_fs  = Fat12/16 bootsector
* struct fat32_bs *fat32 = Fat32 bootsector
* fat_add_t cluster		   = start cluster
***********************************************************************/
fat_clu_list* fat_get_cluster_list(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, fat_add_t cluster)
{
	s32 i, fat_typ;
	u32 count = 0;
	fat_add_t tmp = cluster;
	struct f_entry *add;
	u8 *buf = malloc(0x04);
	u32 *clu_list;
	fat_clu_list *list = NULL;
	
	
	if(fat_fs) {
		fat_typ = get_fat_type(fat_fs);
		
		switch(fat_typ) {
			case 12: 
				if(cluster == 0) {	 
					count = root_size(fat_fs) / clu_size(fat_fs);
					clu_list = malloc(count * sizeof(*clu_list));
					clu_list = malloc(sizeof(*clu_list));
					list = malloc(sizeof(*list));
					list->clu_add = clu_list;
					list->count = count;
					clu_list[0] = 0;
					return list;
				}
			
				for(i = 0;; i++) {	 
					device_read(ctx, buf, 0x02, (storage * SECTOR_SIZE) + fat12_e(fat_fs, tmp));
					
					add = (struct f_entry*)buf; 
					if((tmp % 2) == 0) {add->entry = add->entry & 0x00000FFF;} 
					else {add->entry = add->entry >> 4 & 0x00000FFF;}
					add->entry &= 0x00000FFF;
				
					tmp = add->entry; 
					count++; 
				
					if(add->entry == 0x00000FFE || add->entry == 0x00000FFF) 
						break;
				}
				
				clu_list = malloc(count * sizeof(*clu_list)); 
				list = malloc(sizeof(*list));	 
				list->clu_add = clu_list; 
				list->count = count; 
			
				tmp = cluster; 
					
				for(i = 0; i < count; i++) { 
					clu_list[i] = tmp; 
					
					device_read(ctx, buf, 0x02, (storage * SECTOR_SIZE) + fat12_e(fat_fs, tmp));
					
					if((tmp % 2) == 0) {add->entry = add->entry & 0x00000FFF;}	 
					else {add->entry = add->entry >> 4 & 0x00000FFF;}
					
					add = (struct f_entry*)buf;	 
					tmp = add->entry; 
					
					if(add->entry == 0x00000FFE || add->entry == 0x00000FFF) 
						break;
				}	
			break;
			case 16: 
				if(cluster == 0) {	 
					count = root_size(fat_fs) / clu_size(fat_fs);
					clu_list = malloc(count * sizeof(*clu_list));
					clu_list = malloc(sizeof(*clu_list));
					list = malloc(sizeof(*list));
					list->clu_add = clu_list;
					list->count = count;
					clu_list[0] = 0;
					return list;
				}
				
				for(i = 0;; i++) {	 
					device_read(ctx, buf, 0x02, (storage * SECTOR_SIZE) + fat16_e(fat_fs, tmp));	

					add = ((struct f_entry*)buf); 
					add->entry &= 0x0000FFFF; 
					tmp = add->entry; 
					
					count++; 
					
					if(add->entry == 0x0000FFFE || add->entry == 0x0000FFFF) 
						break;
				}
			
				clu_list = malloc(count * sizeof(*clu_list)); 
				list = malloc(sizeof(*list)); 
				list->clu_add = clu_list; 
				list->count = count;  
			
				tmp = cluster; 
				
				for(i = 0; i < count; i++) {	 
					clu_list[i] = tmp; 
					
					device_read(ctx, buf, 0x02, (storage * SECTOR_SIZE) + fat16_e(fat_fs, tmp));
					
					add = (struct f_entry*)buf;	 
					tmp = add->entry; 
				
					if(add->entry == 0x0000FFFE || add->entry == 0x0000FFFF) 
					break;
				}
			break;
		}	
	
	return list;
	}
	else {  
		
		if(cluster == 0){	 
			cluster = 2;
		}
		
		for(i = 0;; i++){	 
			device_read(ctx, buf, 0x04, (storage * SECTOR_SIZE) + fat32_e(fat32, tmp));
			
			add = ((struct f_entry*)buf);	 
			tmp = add->entry; 
			count++; 
					
			if(add->entry == 0x0FFFFFFE || add->entry == 0x0FFFFFFF) 
				break;
		}
		
		clu_list = malloc(count * sizeof(*clu_list)); 
		list = malloc(sizeof(*list));	 
		list->clu_add = clu_list;	 
		list->count = count;  
			
		tmp = cluster; 
		
		for(i = 0; i < count; i++) {	 
			clu_list[i] = tmp; 
			
			device_read(ctx, buf, 0x04, (storage * SECTOR_SIZE) + fat32_e(fat32, tmp));
			
			add = (struct f_entry*)buf;	 
			tmp = add->entry;	 
				
			if(add->entry == 0x0FFFFFFE || add->entry == 0x0FFFFFFF) 
			break;
		}
		
		return list;
	}
}
/***********************************************************************
* Free a cluster list.
* 
* fat_clu_list *list = cluster list to free
***********************************************************************/
void fat_free_cluster_list(fat_clu_list *list)
{
	free(list->clu_add);
	free(list);
}
/***********************************************************************
* Read all cluster of a chain.
* 
* ps3_context *ctx       = ps3 device information
* u64 storage						 = partition
* struct fat_bs *fat_fs  = Fat12/16 bootsector
* struct fat32_bs *fat32 = Fat32 bootsector
* fat_clu_list *list		 = list of cluster addresses
* u8 *buf                = pointer auf puffer für einzulesende cluster
* u32 start              = start cluster adresse
* u32 count              = cluster anzahl
* 
* return: number of clusters read
***********************************************************************/
s32 fat_read_cluster(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, fat_clu_list *list, u8 *buf, u32 start, u32 count)
{
	s32 i;
	
	
	if(fat_fs) {
		if(list) {
			if(list->clu_add[0] == 0) { 
				for(i = 0; i < list->count; i++)
					device_read(ctx, buf + (i * clu_size(fat_fs)), clu_size(fat_fs), (storage * SECTOR_SIZE) + root_off(fat_fs) + (i * clu_size(fat_fs)));
				return i;
			}
			
			if(list->clu_add[0] >= 2) {  
				for(i = 0; i < list->count; i++)
					device_read(ctx, buf + (i * clu_size(fat_fs)), clu_size(fat_fs), (storage * SECTOR_SIZE) + clu_off(fat_fs, list->clu_add[i]));
				return i;	
			}
		}
		
		for(i = 0; i < count; i++)
			device_read(ctx, buf + (i * clu_size(fat_fs)), clu_size(fat_fs), (storage * SECTOR_SIZE) + clu_off(fat_fs, start));
		return i;
	}
	else {
		if(list) {
			if(list->clu_add[0] >= 2) { 
				for(i = 0; i < list->count; i++)
					device_read(ctx, buf + (i * clu_size32(fat32)), clu_size32(fat32), (storage * SECTOR_SIZE) + clu_off32(fat32, list->clu_add[i]));
				return i;	
			}
		}
		
		for(i = 0; i < count; i++)
			device_read(ctx, buf + (i * clu_size32(fat32)), clu_size32(fat32), (storage * SECTOR_SIZE) + clu_off32(fat32, start));
		
		return i;
	}
}
/***********************************************************************
* Determines the number of clusters in a dir.
* 
* u8 *clusters = entry data
* 
* return: number of clusters
***********************************************************************/
s32 fat_get_entry_count(u8 *clusters)
{
	s32 i;
	u8 c, cs1, cs2, seq;
	u8 *p = clusters;
	u8 *tmp = malloc(0x20);
	struct sfn_e *sfn;
	
	c = cs1 = cs2 = seq = 0;
	
	
	while(*p != 0x00) {  
		if(*p != 0xE5) { 
			p += 0x0B; 
			
			if(*p == 0x0F) {	 
				p += 0x02; 
				cs1 = *p; 
				p -= 0x0D; 
				seq = *p & 0x1F; 
			
				for(i = 0; i < seq; i++) {	 
					p += 0x0D; 
				
					if(*p == cs1) { 
						p -= 0x0D; 
						p += 0x20; 
					}
				}
				memcpy(tmp, p, 0x20); 
				sfn = (struct sfn_e*)tmp;
			
				for (i = 0; i < 11; i++) { 
					cs2 = ((cs2 & 0x01) ? 0x80 : 0) + (cs2 >> 1);
					cs2 = cs2 + sfn->d_name[i];
				}
			
				if(cs2 == cs1) { 
					c++; 
				}
				cs2 = 0;
			}
			else { 
				p -= 0x0B; 
				c++; 
			}
			p += 0x20; 
		}
		else {
			p += 0x20; 
		}
	}
	
	return c; 
}
/***********************************************************************
* get LFN entrie name
***********************************************************************/
s32 get_lfn_name(u8 *tmp, char part[M_LFN_LENGTH])
{
	s32 i;
	
	part[0]  = tmp[0x01];
	part[1]  = tmp[0x03];
	part[2]  = tmp[0x05];
	part[3]  = tmp[0x07];
	part[4]  = tmp[0x09];
	part[5]  = tmp[0x0E];
	part[6]  = tmp[0x10];
	part[7]  = tmp[0x12];
	part[8]  = tmp[0x14];
	part[9]  = tmp[0x16];
	part[10] = tmp[0x18];
	part[11] = tmp[0x1C];
	part[12] = tmp[0x1E];
  
	for(i = 0; i < M_LFN_LENGTH; i++)
		if(part[i] == 0xFF) 
			part[i] = 0x20;
	
	return 0;
}
/***********************************************************************
* sfn name aufbereiten
***********************************************************************/
s32 get_sfn_name(u8 *sfn_name, u8 *name)
{
	s32 i;
	u8 tmp_nam[9] = {0x20};	 
	u8 tmp_ext[4] = {0x20};	 
	
	for(i = 0; i < 260; i++) 
				name[i] = 0x00;
	
	memcpy(tmp_nam, sfn_name, 8);  
	for(i = 0; i < 9; i++)  
		if(tmp_nam[i] == 0x20) 
			tmp_nam[i] = 0x00; 
	
	if(sfn_name[8] == 0x20) { 
		memcpy(name, tmp_nam, strlen((const char*)tmp_nam)); 
		return 0;
	}
	else { 
		memcpy(tmp_ext, sfn_name + 8, 3); 
		sprintf((char*)name, "%s.%s", tmp_nam, tmp_ext); 
		return 0;
	}
	
	return -1;
}
/***********************************************************************
* Funktion: fat_get_entry_list, erstellt eine puffer mit allen entries
* plus daten.
* 	u8 *cluster = fat-cluster/s mit entry daten.
* 	u8 *name    = entry name der gesucht wird.
***********************************************************************/
s32 fat_dir_search_entry(u8 *cluster, u8 *s_name, u8 *tmp)
{
	s32 i, x, y;
	u16 n_len;
	u8 *e_ptr = cluster;
	u8 cs_1, cs_2, seq;
	char part[M_LFN_LENGTH] = {0};
	char lfname[M_LFN_ENTRIES][M_LFN_LENGTH];
	char name[260];
	struct sfn_e *sfn;
	
	cs_1 = cs_2 = seq = n_len = 0;
	for(x = 0; x < M_LFN_ENTRIES; x++)
		for(y = 0; y < M_LFN_LENGTH; y++)
			lfname[x][y] = 0x00;
	
	e_ptr = cluster;
	while(*e_ptr != 0x00) { 
		if(*e_ptr != 0xE5) { 
			e_ptr += 0x0B; 
		
		if(*e_ptr == 0x0F) {	 
			e_ptr += 0x02; 
			cs_2 = *e_ptr; 
			e_ptr -= 0x0D; 
			seq = *e_ptr & 0x1F; 
			
			for(i = 0; i < seq; i++) { 
				memcpy(tmp, e_ptr, 0x20);	 
				
				if(tmp[0x0D] == cs_2) { 
					get_lfn_name(tmp, part);
					strncpy(lfname[seq - 1 - i], part, 13);
					e_ptr += 0x20;
				}
			}
			
			memcpy(tmp, e_ptr, 0x20); 
			sfn = (struct sfn_e*)tmp;
			
			for (i = 0; i < 11; i++) { 
				cs_1 = ((cs_1 & 0x01) ? 0x80 : 0) + (cs_1 >> 1);
				cs_1 = cs_1 + sfn->d_name[i];
			}

			if(cs_1 == cs_2) {	 
				cs_1 = 0;						 
				memcpy(name, lfname, 260); 
				
				for(x = 0; x < M_LFN_ENTRIES; x++) 
					for(y = 0; y < M_LFN_LENGTH; y++)
						lfname[x][y] = 0x00;
				
				if(strcmp(name, (const char *)s_name) == 0) { 
					return sfn->d_start_clu;
				}
			}
		}
		else if(*e_ptr != 0x0F) { 
			e_ptr -= 0x0B;
			memcpy(tmp, e_ptr, 0x20); 
			sfn = (struct sfn_e*)tmp;
			get_sfn_name(sfn->d_name, (u8*)name); 
			
			if(strcmp(name, (const char *)s_name) == 0) { 
				return sfn->d_start_clu;
			}
		}
			e_ptr += 0x20; 
		} 
		else {
			e_ptr += 0x20;
		}
		
	}
	return -1;
}
/***********************************************************************
* Funktion: fat_lookup_path, gibt cluster-adresse eines files oder dirs
* 					zurück, oder 0 wenn file/dir nicht existiert.
* 	HANDLE device         = device-handle
* 	u64 storage						= partition
* 	struct fat_bs *fat_fs = struct fat_fs
* 	u8 *path 		    	    = gesuchter pfad
* 	fat_add_t fat_root    = root-verzeichniss, ab diesem verzeichniss
* 													soll gesucht werden. wenn 0, wird vom ROOT
* 													des fs ausgegangen.
***********************************************************************/
struct sfn_e* fat_lookup_path(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, u8 *path, fat_add_t fat_root)
{
	s32 ret;
	u8 *str, *n_str, *clu_data;
	fat_add_t cluster;
	fat_add_t tmp_clu = 0; 
	fat_clu_list *list; 
	struct sfn_e *entry;
	u8 *s_entry = malloc(0x20);
	
	
	str = malloc(MAX_PATH); 
	
	if(fat_fs) {
		if(path[0] == '/') {	 
			memcpy(str, &path[1], strlen((char*)path));	 
			cluster = 0; 
		}
    
	  for(n_str = str; n_str && n_str[0]; n_str = str) {
		  if((str = memchr(str, 0x2F, strlen((char*)str)))) {
			  *str = '\0';
			  str++;
		  }
    
		  tmp_clu = cluster;
      
		  list = fat_get_cluster_list(ctx, storage, fat_fs, 0, tmp_clu);	 
		  clu_data = malloc(list->count * clu_size(fat_fs));	 
		  fat_read_cluster(ctx, storage, fat_fs, 0, list, clu_data, 0, 0); 
		  fat_free_cluster_list(list); 
			
		  ret = fat_dir_search_entry(clu_data, n_str, s_entry);
			
		  if(ret != -1) { 
			  cluster = ret; 
		  }
		  else {	 
			  free(clu_data);
			  return NULL;
		  }	
	  }
	  
	  entry = (struct sfn_e*)s_entry; 
	  
	  free(clu_data);
	  return entry;
	}
	else {
		
		if(path[0] == '/') {	 
			memcpy(str, &path[1], strlen((char*)path)); 
			cluster = 2; 
		}

		for(n_str = str; n_str && n_str[0]; n_str = str) {
			if((str = memchr(str, 0x2F, strlen((char*)str)))) {
				*str = '\0';
				str++;
			}
			
			tmp_clu = cluster;
      
			list = fat_get_cluster_list(ctx, storage, 0, fat32, tmp_clu); 
			clu_data = malloc(list->count * clu_size32(fat32)); 
			fat_read_cluster(ctx, storage, 0, fat32, list, clu_data, 0, 0); 
			fat_free_cluster_list(list); 
			
			ret = fat_dir_search_entry(clu_data, n_str, s_entry);
			
			if(ret != -1) { 
				cluster = ret;
			}
			else { 
				free(clu_data);
				return NULL;
			}
		}
		
		entry = (struct sfn_e*)s_entry; 
		
		if(entry->d_start_clu == 0)
			entry->d_start_clu = 2;
		
		free(str);
		free(clu_data);
		return entry;
	}
}
/***********************************************************************
* Funktion: fat_get_entries, erstellt eine puffer mit metadaten aller
* entries.
* 	u8 *in  									 = puffer mit roh entry daten.
* 	struct fat_dir_entry *dirs = struct array für entries.
***********************************************************************/
s32 fat_get_entries(u8 *buf, struct fat_dir_entry *dirs)
{
	s32 i, k, l, m;
	u8 cs_1, cs_2, sq;
	u8 *e_ptr = buf;
	u8 *tmp = malloc(0x20);
	char lfname[M_LFN_ENTRIES][M_LFN_LENGTH];
	char part[M_LFN_LENGTH] = {0};
	char name[260];
	struct sfn_e *sfn;
	
	e_ptr = buf;
	
	cs_1 = cs_2 = sq = m = 0;
	for(k = 0; k < M_LFN_ENTRIES; k++)
		for(l = 0; l < M_LFN_LENGTH; l++)
			lfname[k][l] = 0x00;
	
	while(*e_ptr != 0x00) {  
		if(*e_ptr != 0xE5) { 
			e_ptr += 0x0B; 
			
			if(*e_ptr == 0x0F) { 
				e_ptr += 0x02; 
				cs_2 = *e_ptr; 
				e_ptr -= 0x0D; 
				sq = *e_ptr & 0x1F; 
			
				for(i = 0; i < sq; i++) { 
					memcpy(tmp, e_ptr, 0x20); 
				
					if(tmp[0x0D] == cs_2) { 
						get_lfn_name(tmp, part); 
						strncpy(lfname[sq - 1 - i], part, 13);
						e_ptr += 0x20;
					}
				}
				
				memcpy(tmp, e_ptr, 0x20); 
				sfn = (struct sfn_e*)tmp;
				
				for (i = 0; i < 11; i++) { 
					cs_1 = ((cs_1 & 0x01) ? 0x80 : 0) + (cs_1 >> 1);
					cs_1 = cs_1 + sfn->d_name[i];
				}
				
				if(cs_1 == cs_2) { 
					cs_1 = 0; 
					memcpy(name, lfname, 260); 
					
					for(k = 0; k < M_LFN_ENTRIES; k++) 
						for(l = 0; l < M_LFN_LENGTH; l++)
							lfname[k][l] = 0x00;
					
					memcpy(dirs[m].fd_dos_name, sfn->d_name, 11);
					dirs[m].fd_att 			 = sfn->d_att;
					dirs[m].fd_case 		 = sfn->d_case;
					dirs[m].fd_ctime_ms  = sfn->d_ctime_ms;
					dirs[m].fd_ctime 		 = sfn->d_ctime;
					dirs[m].fd_cdate 		 = sfn->d_cdate;
					dirs[m].fd_atime 		 = sfn->d_atime;
					dirs[m].fd_adate 	   = sfn->d_adate;
					dirs[m].fd_mtime 	   = sfn->d_mtime;
					dirs[m].fd_mdate 	   = sfn->d_mdate;
					dirs[m].fd_start_clu = sfn->d_start_clu;
					dirs[m].fd_size      = sfn->d_size;
					memcpy(dirs[m].fd_name, name, 261);
					m++;
				}
			}
			else {
				e_ptr -= 0x0B;
				memcpy(tmp, e_ptr, 0x20);	 
				sfn = (struct sfn_e*)tmp; 
				memset(name, 0x00, 260); 
				get_sfn_name(sfn->d_name, (u8*)name); 
				
				memcpy(dirs[m].fd_dos_name, sfn->d_name, 11);
				dirs[m].fd_att 			 = sfn->d_att;
				dirs[m].fd_case 		 = sfn->d_case;
				dirs[m].fd_ctime_ms  = sfn->d_ctime_ms;
				dirs[m].fd_ctime 		 = sfn->d_ctime;
				dirs[m].fd_cdate 		 = sfn->d_cdate;
				dirs[m].fd_atime 		 = sfn->d_atime;
				dirs[m].fd_adate 	   = sfn->d_adate;
				dirs[m].fd_mtime 	   = sfn->d_mtime;
				dirs[m].fd_mdate 	   = sfn->d_mdate;
				dirs[m].fd_start_clu = sfn->d_start_clu;
				dirs[m].fd_size      = sfn->d_size;
				memcpy(dirs[m].fd_name, name, 261);
				m++;
			}
			e_ptr += 0x20; 
		}
		else {
			e_ptr += 0x20; 
		}
	}
	
	return 0;
}
/***********************************************************************
* sortier function für directory entries
***********************************************************************/
s32 sort_dir(const void *first, const void *second)
{
	const struct fat_dir_entry *a = first;
	const struct fat_dir_entry *b = second;
	
	
	if(!strcmp((const char *)a->fd_name, ".")) {
		return -1;
	}
	else if(!strcmp((const char *)b->fd_name, ".")) {
		return 1;
	}
	else if(!strcmp((const char *)a->fd_name, "..")) {
		return -1;
	}
	else if(!strcmp((const char *)b->fd_name, "..")) {
		return 1;
	}
	else {
		return strcmp((const char *)a->fd_name, (const char *)b->fd_name);
	}
}
/***********************************************************************
* date time
***********************************************************************/
struct date_time fat_datetime_from_entry(u16 date, u16 time)
{
	struct date_time t;
	u8 day, month, year, hour, minutes, seconds;
	day = month = year = hour = minutes = seconds = 0;
	
	
	day     = (date & 0x001F) >>  0;
	month   = (date & 0x01E0) >>  5;
	year    = (date & 0xFE00) >>  9;
	hour    = (time & 0xF800) >> 11;
	minutes = (time & 0x07E0) >>  5;
	seconds = (time & 0x001F) <<  1;
	
	t.day = day;	
	t.month = month;	 
	t.year = year;	
	t.hour = hour;	
	t.minutes = minutes;	
	t.seconds = seconds;
	
	return t;
}
/***********************************************************************
* Funktion: fat_print_dir_list, listet die entries eines dir auf.
* 	HANDLE device         = device-handle
* 	u64 storage						= partitions start sektor
* 	struct fat_bs *fat_fs = struct fat_fs
* 	u8 *path 		    	    = pfad zum directory
* 	u8 *volume						= partition
***********************************************************************/
s32 fat_print_dir_list(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, u8 *path, u8 *volume, u64 free_byte)
{	
	s32 i, entry_count;
	u8 *buffer;
	u64 file_count, dir_count, byte_use, byte_free;
	struct sfn_e *dir;
	fat_clu_list *list;
	struct fat_dir_entry *dirs;
	struct date_time dt;
	char timestr[64];
	char sizestr[32];
	char byte_use_str[32];
	char byte_free_str[32];
	file_count = dir_count = byte_use = byte_free = 0;
	
	
	// welches dateisystem ?
	
  
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	if(fat_fs) {
		if((strlen((char*)path)) == 1 && path[0] == 0x2F) {                       // if path == root
			list = fat_get_cluster_list(ctx, storage, fat_fs, 0, 0); 
			buffer = malloc(list->count * clu_size(fat_fs)); 
			fat_read_cluster(ctx, storage, fat_fs, 0, list, buffer, 0, 0); 
			fat_free_cluster_list(list); 
			entry_count = fat_get_entry_count(buffer);	 
			dirs = malloc(entry_count * sizeof(*dirs)); 
			fat_get_entries(buffer, dirs); 
			
		}
		else {
			if((dir = fat_lookup_path(ctx, storage, fat_fs, 0, path, 0)) == NULL) {
				printf("no such file or directory!\n");
				return -1;
			}
      
			if((dir->d_att & A_DIR) != A_DIR) {
				printf("can't show, not a directory!\n");
				return -1;
			}
		  
			list = fat_get_cluster_list(ctx, storage, fat_fs, 0, dir->d_start_clu);	
			buffer = malloc(list->count * clu_size(fat_fs));	
			fat_read_cluster(ctx, storage, fat_fs, 0, list, buffer, 0, 0);	
			fat_free_cluster_list(list);	
			entry_count = fat_get_entry_count(buffer);	
			dirs = malloc(entry_count * sizeof(*dirs));	
			fat_get_entries(buffer, dirs);	
		}
	}
	else {
		if((strlen((char*)path)) == 1 && path[0] == 0x2F) {
			list = fat_get_cluster_list(ctx, storage, 0, fat32, fat32->bs32_rootclu);	
			buffer = malloc(list->count * clu_size32(fat32));	
			fat_read_cluster(ctx, storage, 0, fat32, list, buffer, 0, 0);	
			fat_free_cluster_list(list);	
			entry_count = fat_get_entry_count(buffer);	
			dirs = malloc(entry_count * sizeof(*dirs));	
			fat_get_entries(buffer, dirs);	
		}
		else {
			if((dir = fat_lookup_path(ctx, storage, 0, fat32, path, 0)) == NULL) {
				printf("no such file or directory!\n");
				return -1;
			}
		  
			if((dir->d_att & A_DIR) != A_DIR) {
				printf("can't show, not a directory!\n");
				return -1;
			}
			
			list = fat_get_cluster_list(ctx, storage, 0, fat32, dir->d_start_clu);
			buffer = malloc(list->count * clu_size32(fat32));
			fat_read_cluster(ctx, storage, 0, fat32, list, buffer, 0, 0);
			fat_free_cluster_list(list);
			entry_count = fat_get_entry_count(buffer);
			dirs = malloc(entry_count * sizeof(*dirs));
			fat_get_entries(buffer, dirs);
		}
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	free(buffer);	 
	
	qsort(dirs, entry_count, sizeof(*dirs), sort_dir); 
	
	printf("\n Volume is ps3_hdd %s\n", volume);
	printf(" Directory of %s/%s\n\n", volume, path);
	
	for(i = 0; i < entry_count; ++i) { 
		dt = fat_datetime_from_entry(dirs[i].fd_mdate, dirs[i].fd_mtime); 
		sprintf(timestr, "%02d.%02d.%04d  %02d:%02d", 
				dt.month, dt.day, dt.year + 1980, dt.hour, dt.minutes);
		
		if((dirs[i].fd_att & A_DIR) == A_DIR) { 
			dir_count++;
			sprintf(sizestr, "<DIR>"); 
			printf("%s    %-14s %s\n", timestr, sizestr, dirs[i].fd_name);
		}
		else {
			file_count++;
			byte_use = byte_use + dirs[i].fd_size;
			print_commas(dirs[i].fd_size, sizestr);
			printf("%s    %14s %s\n", timestr, sizestr, dirs[i].fd_name); 
		}	
	}
	print_commas(byte_use, byte_use_str);
	print_commas(free_byte, byte_free_str);
	printf("%16llu File(s),  ", file_count);
	printf("%s bytes\n", byte_use_str);
	printf("%16llu Dir(s),  ", dir_count);
	printf("%s bytes free\n", byte_free_str);
	
	free(dirs);
	return 0;
}
/***********************************************************************
* Funktion: fat_copy_data, kopiert files/folders ins programmverzeichnis.
* 	HANDLE device          = device-handle
* 	u64 storage						 = partitions start sektor
* 	struct fat_bs *fat_fs  = struct fat_fs, wenn FAT12/16
* 	struct fat32_bs *fat32 = struct fat32_bs, wenn FAT32
* 	u8 *srcpath 		    	 = quelle
* 	u8 *destpath					 = ziel
***********************************************************************/
s32 fat_copy_data(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, char *srcpath, char *destpath)
{
	s32 i, entry_count;
	u8 *buf;
	char *tmp;
	s64 totalsize, readsize, read;
	char string[MAX_PATH];
	char newdest[MAX_PATH];
	int using_con;
	struct sfn_e *dir = NULL;	
	fat_clu_list *list = NULL;
	struct fat_dir_entry *dirs;
	struct utimbuf filetime;
	FILE *out;
	
	
	if(strlen(srcpath) == 1 && srcpath[0] == 0x2F)
		return 0;
	
	using_con = (destpath && (!Stricmp((const char *)destpath, "CON") || !(Strnicmp((const char *)destpath, "CON.", 4))));
  
	if(fat_fs) {
		if((dir = fat_lookup_path(ctx, storage, fat_fs, 0, (u8*)srcpath, 0)) == NULL) {
			printf("can't copy, no such file or directory!\n");
			return -1;
		}
	}
	else if(fat32) {
		if((dir = fat_lookup_path(ctx, storage, 0, fat32, (u8*)srcpath, 0)) == NULL) {
			printf("can't copy, no such file or directory!\n");
			return -1;
		}
	}
	
	if(destpath == NULL) {	 
		char *base = basename((const char *)srcpath);
		strcpy(newdest, "./"); 
		strcat(newdest, base);
		free(base);
	}
	else {	
		while(destpath[strlen((const char *)destpath) - 1] == '/')
			destpath[strlen((const char *)destpath) - 1] = '\0';
	  
		strcpy(newdest, (const char *)destpath);
	}
	
	totalsize = dir->d_size;
	
	if((dir->d_att & A_DIR) == A_DIR) {
		char *dirdest;
		char nextsrc[256] = {0x00};
		char nextdest[MAX_PATH] = {0x00};
		struct stat sb;
		
		if(using_con) {
			return -1;
		}
		
		if(!stat(newdest, &sb)) {
			if((sb.st_mode & S_IFMT) != S_IFDIR) {
				return -1;
			}
			dirdest = strdup(newdest);
		}
		else {
			if(mkdir(newdest, 0777) == -1) {
				dirdest = valid_filename(newdest, 0);
				if(mkdir(dirdest,0777) == -1) {
					free(dirdest);
					return -1;
				}
			}
			else {
				dirdest = strdup(newdest);
			}
		}
		 
		if(fat_fs) {
			list = fat_get_cluster_list(ctx, storage, fat_fs, 0, dir->d_start_clu); 
			buf = malloc(list->count * clu_size(fat_fs)); 
			fat_read_cluster(ctx, storage, fat_fs, 0, list, buf, 0, 0); 
		}
		else if(fat32) {
			list = fat_get_cluster_list(ctx, storage, 0, fat32, dir->d_start_clu); 
			buf = malloc(list->count * clu_size32(fat32)); 
			fat_read_cluster(ctx, storage, 0, fat32, list, buf, 0, 0); 
		}
		
		fat_free_cluster_list(list); 
		entry_count = fat_get_entry_count(buf); 
		dirs = malloc(entry_count * sizeof(*dirs)); 
		fat_get_entries(buf, dirs); 
		free(buf);
		
		for(i = 0; i < entry_count; ++i) {
			if(!strcmp((const char *)dirs[i].fd_name, ".") || !strcmp((const char *)dirs[i].fd_name, ".."))	 
				continue;
			
			strcpy(nextsrc, (const char *)srcpath);
			
			if(srcpath[strlen((const char *)srcpath) - 1] != '/')
				strcat(nextsrc, "/");
				
			strcat(nextsrc, (const char *)dirs[i].fd_name);
			strcpy(nextdest, dirdest);
			strcat(nextdest, "/");
			strcat(nextdest, (const char *)dirs[i].fd_name);
			sprintf(string, "%s/%s", srcpath, dirs[i].fd_name);
			printf("copy -> %s\n", string);
			
			if(fat_fs) {
				fat_copy_data(ctx, storage, fat_fs, 0, nextsrc, nextdest);
			}
			else if(fat32) {
				fat_copy_data(ctx, storage, 0, fat32, nextsrc, nextdest);
			}
		}
		free(dirs);
		return 0;
	}
	
	readsize = 0;
	read = 0;
	
	tmp = valid_filename(newdest, using_con);
	
	strcpy(newdest, (const char *)tmp);
	free(tmp);
	
	out = fopen(newdest, "wb"); 
	if(!out){	 				
		printf("can't create file! \"%s\"\n", newdest); 
		return -1;
	}
	
	if(fat_fs) {
		buf = malloc(clu_size(fat_fs)); 
		list = fat_get_cluster_list(ctx, storage, fat_fs, 0, dir->d_start_clu); 
	
		for(i = 0; readsize < totalsize; i++) { 
			if(totalsize - readsize < clu_size(fat_fs)) {
				read = device_read(ctx, buf, totalsize - readsize, (storage * SECTOR_SIZE) + clu_off(fat_fs, list->clu_add[i]));
				fwrite(buf, 1, read, out);
				readsize += read;
			}
			else {
				read = device_read(ctx, buf, clu_size(fat_fs), (storage * SECTOR_SIZE) + clu_off(fat_fs, list->clu_add[i]));
				fwrite(buf, 1, read, out);
				readsize += read;
			}
			
			fprintf(stderr,"(%03lld%%)\r", readsize * 100 / totalsize);
		}
	}
	else if(fat32) {
		buf = malloc(clu_size32(fat32));	
		list = fat_get_cluster_list(ctx, storage, 0, fat32, dir->d_start_clu);	
	
		for(i = 0; readsize < totalsize; i++) {	
			if(totalsize - readsize < clu_size32(fat32)) {
				read = device_read(ctx, buf, totalsize - readsize, (storage * SECTOR_SIZE) + clu_off32(fat32, list->clu_add[i]));
				fwrite(buf, 1, read, out);
				readsize += read;
			}
			else {
				read = device_read(ctx, buf, clu_size32(fat32), (storage * SECTOR_SIZE) + clu_off32(fat32, list->clu_add[i]));
				fwrite(buf, 1, read, out);
				readsize += read;
			}

			fprintf(stderr,"(%03lld%%)\r", readsize * 100 / totalsize);
		}
	}
	
	fat_free_cluster_list(list); 
	free(buf);						
	fclose(out);
	
	filetime.actime = fat2unix_time(dir->d_atime, dir->d_adate); 
	filetime.modtime = fat2unix_time(dir->d_mtime, dir->d_mdate); 
	utime(newdest, &filetime); 
	
	return 0;
}

/***********************************************************************
* Funktion: fat_copy_data, kopiert files/folders ins programmverzeichnis.
* 	HANDLE device          = device-handle
* 	u64 storage						 = start sector of partition
* 	struct fat_bs *fat_fs  = struct fat_fs, wenn FAT12/16
* 	struct fat32_bs *fat32 = struct fat32_bs, wenn FAT32
* 	u8 *srcpath 		    	 = quelle
* 	u8 *destpath					 = ziel
***********************************************************************/
s32 fat_replace_data(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, char *path)
{
	s32 i, size;
	u8 *buf;
	s64 totalsize, done, n_write;
	struct sfn_e *dir = NULL;	
	fat_clu_list *list = NULL;
	u8 *buf_fd = NULL;
	FILE *fd;
	
	if(strlen(path) == 1 && path[0] == 0x2F)
		return 0;
	
	if(fat_fs) {
		if((dir = fat_lookup_path(ctx, storage, fat_fs, 0, (u8*)path, 0)) == NULL) {
			printf("can't copy, no such file or directory!\n");
			return -1;
		}
	}
	else if(fat32) {
		if((dir = fat_lookup_path(ctx, storage, 0, fat32, (u8*)path, 0)) == NULL) {
			printf("can't copy, no such file or directory!\n");
			return -1;
		}
	}
	
	totalsize = dir->d_size;
	 
	if((fd = fopen(strrchr(path, '/') + 1, "rb")) == NULL) {
		printf("can't open patched file!\n");
		return 0;
	}
	
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if(size != totalsize) {
		printf("file size wrong!\n");
	  return 0;
	}
	
	done    = 0;
	n_write = 0;
	
	if(fat_fs)
	  buf_fd = malloc(clu_size(fat_fs));
  if(fat32)
    buf_fd = malloc(clu_size32(fat32));
	
	if(fat_fs) {
		buf = malloc(clu_size(fat_fs)); 
		list = fat_get_cluster_list(ctx, storage, fat_fs, 0, dir->d_start_clu); 
	
		for(i = 0; done < totalsize; i++) { 
			if(totalsize - done < clu_size(fat_fs)) {
				fseek(fd, done, SEEK_SET);
				fread(buf_fd, sizeof(u8), totalsize - done, fd);
				n_write = device_write(ctx, buf_fd, totalsize - done, (storage * SECTOR_SIZE) + clu_off(fat_fs, list->clu_add[i]));
				done += n_write;
			}
			else {
				fseek(fd, done, SEEK_SET);
			  fread(buf_fd, sizeof(u8), clu_size(fat_fs), fd);
				n_write = device_write(ctx, buf_fd, clu_size(fat_fs), (storage * SECTOR_SIZE) + clu_off(fat_fs, list->clu_add[i]));
				done += n_write;
			}
			fprintf(stderr,"(%03lld%%)\r", done * 100 / totalsize);
		}
	}
	else if(fat32) {
		buf = malloc(clu_size32(fat32));	
		list = fat_get_cluster_list(ctx, storage, 0, fat32, dir->d_start_clu);	
	
		for(i = 0; done < totalsize; i++) {	
			if(totalsize - done < clu_size32(fat32)) {
				fseek(fd, done, SEEK_SET);
				fread(buf_fd, sizeof(u8), totalsize - done, fd);
				n_write = device_write(ctx, buf_fd, totalsize - done, (storage * SECTOR_SIZE) + clu_off32(fat32, list->clu_add[i]));
				done += n_write;
			}
			else {
				fseek(fd, done, SEEK_SET);
			  fread(buf_fd, sizeof(u8), clu_size32(fat32), fd);
				n_write = device_write(ctx, buf_fd, clu_size32(fat32), (storage * SECTOR_SIZE) + clu_off32(fat32, list->clu_add[i]));
				done += n_write;
			}
			fprintf(stderr,"(%03lld%%)\r", done * 100 / totalsize);
		}
	}
	
	fat_free_cluster_list(list); 
	free(buf);						
	fclose(fd);
	
	return 0;
}









