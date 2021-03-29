/*
* Copyright (c) 2019 by 3141card
* This file is released under the GPLv2.
*/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "types.h"
#include "util.h"
#include "aes.h"
#include "aes_xts.h"
#include "fs/fat.h"
#include "fs/ufs.h"


s32 get_device_handle(ps3_context *ctx, u8 mode);
s64 block_read(ps3_context *ctx, u8 *buf, s64 n_sec, s64 sec_num);
s64 block_write(ps3_context *ctx, u8 *buf, s64 n_sec, s64 sec_num);
s64 device_read(ps3_context *ctx, u8 *buf, u64 numbytes, s64 dev_off);
s64 device_write(ps3_context *ctx, u8 *buf, u64 numbytes, s64 dev_off);
s32 get_partitions(ps3_context *ctx);

#endif  // _DEVICE_H_
