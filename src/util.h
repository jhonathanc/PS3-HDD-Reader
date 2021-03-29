/*
* Copyright (c) 2011-2012 by ps3dev.net
* This file is released under the GPLv2.
*/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>

#include "types.h"

void _hexdump(FILE *fp, const char *name, u32 offset, u8 *buf, int len, BOOL print_addr);
u8 *_read_buffer(s8 *file, u32 *length);
void _write_buffer(s8 *file, u8 *buffer, u32 length);
void _es16_buffer(u8 *buf, u32 length);
int _print_buf(u8 *b, u64 s, u64 c);
s64 seek_device(HANDLE device, s64 byte_offset);
void print_commas(s64 n, char *out);


#endif
