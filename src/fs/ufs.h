/*
* Copyright (c) 2019 by 3141card
* This file is released under the GPLv2.
*/

#ifndef _UFS2_H_
#define _UFS2_H_

#include "../types.h"
#include "../device.h"
#include "../util.h"
#include "ufs/dinode.h"
#include "ufs/fs.h"
#include "ufs/dir.h"
#include "ufs/ps3.h"
#include "misc.h"

struct fs* ufs_init(ps3_context *ctx);
ufs_inop ufs_lookup_path(ps3_context *ctx, struct fs* ufs2, u8* path, int follow, ufs_inop root_ino);
s32 ufs_print_dir_list(ps3_context *ctx, struct fs* ufs2, u8* path, u8* volume);
s32 ufs_copy_data(ps3_context *ctx, struct fs *ufs2, ufs_inop root_ino, ufs_inop ino, char *srcpath, char *destpath);
s32 ufs_replace_data(ps3_context *ctx, struct fs *ufs2, ufs_inop root_ino, ufs_inop ino, char *path);

#endif  // _UFS2_H_
