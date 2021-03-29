/*
* Copyright (c) 2019 by 3141card
* This file is released under the GPLv2.
*/

#ifndef _FAT_H_
#define _FAT_H_

#include "../types.h"
#include "../device.h"
#include "../util.h"
#include "fat/fatfs.h"
#include "misc.h"

struct fat_bs* init_fat_old(ps3_context *ctx, u64 start);
struct fat32_bs* init_fat32(ps3_context *ctx, u64 start);
s32 get_fat_type(struct fat_bs *fat);
u64 fat_how_many_free_bytes(ps3_context *ctx, u64 storage, struct fat_bs *fat, struct fat32_bs *fat32);
fat_clu_list* fat_get_cluster_list(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, fat_add_t cluster);
void fat_free_cluster_list(fat_clu_list *list);
s32 fat_read_cluster(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, fat_clu_list *list, u8 *buf, u32 start, u32 count);
s32 fat_get_entry_count(u8 *clusters);
s32 get_lfn_name(u8 *tmp, char part[M_LFN_LENGTH]);
s32 get_sfn_name(u8 *sfn_name, u8 *name);
s32 fat_dir_search_entry(u8 *cluster, u8 *s_name, u8 *tmp);
struct sfn_e* fat_lookup_path(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, u8 *path, fat_add_t fat_root);
s32 fat_get_entries(u8 *buf, struct fat_dir_entry *dirs);
s32 sort_dir(const void *first, const void *second);
struct date_time fat_datetime_from_entry(u16 date, u16 time);
s32 fat_print_dir_list(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, u8 *path, u8 *volume, u64 free_byte);
s32 fat_copy_data(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, char *srcpath, char *destpath);
s32 fat_replace_data(ps3_context *ctx, u64 storage, struct fat_bs *fat_fs, struct fat32_bs *fat32, char *path);

#endif  // _FAT_H_
