/*
* Copyright (c) 2011-2012 by ps3dev.net
* This file is released under the GPLv2.
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "types.h"

void _hexdump(FILE *fp, const char *name, u32 offset, u8 *buf, int len, BOOL print_addr)
{
	int i, j, align = strlen(name) + 1;

	fprintf(fp, "%s ", name);
	if(print_addr == TRUE)
		fprintf(fp, "%08x: ", offset);
	for(i = 0; i < len; i++)
	{
		if(i % 16 == 0 && i != 0)
		{
			fprintf(fp, "\n");
			for(j = 0; j < align; j++)
				putchar(' ');
			if(print_addr == TRUE)
				fprintf(fp, "%08X: ", offset + i);
		}
		fprintf(fp, "%02X ", buf[i]);
	}
	fprintf(fp, "\n");
}

u8 *_read_buffer(s8 *file, u32 *length)
{
	FILE *fp;
	u32 size;

	if((fp = fopen(file, "rb")) == NULL)
		return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	u8 *buffer = (u8 *)malloc(sizeof(u8) * size);
	fread(buffer, sizeof(u8), size, fp);

	if(length != NULL)
		*length = size;

	fclose(fp);

	return buffer;
}

void _write_buffer(s8 *file, u8 *buffer, u32 length)
{
	FILE *fp;

	if((fp = fopen(file, "wb")) == NULL)
		return;

	fwrite(buffer, sizeof(u8), length, fp);
	fclose(fp);
}

/***********************************************************************
*
***********************************************************************/
void _es16_buffer(u8 *buf, u32 length)
{
	u32 i;
  u64 *p = (u64*)buf;
  
	for(i = 0; i < length / 8; i++)
    p[i] = (p[i] & 0xFF00FF00FF00FF00) >> 8 | (p[i] & 0x00FF00FF00FF00FF) << 8;
  
  
}

/***********************************************************************
*DEBUG: print a buffer in hex view
* 	u8 *b  = buffer to print
* 	u64 s  = count of lines to skip from begin
* 	u64 c  = count of lines to show	
***********************************************************************/
int _print_buf(u8 *b, u64 s, u64 c)
{
	int i, k;
	
	
	printf("-------------------------------------------------------------------\n");
	for(i = s; i < c + s; i++){
		for(k = 0; k < 16; k++){
			if(k == 7){
				printf("%02X  ", b[ k + (i *16)]);
			}
			else{
				printf("%02X ", b[ k + (i *16)]);
			}
		}
		
		printf("|");
		
		for(k = 0; k < 16; k++){
			if((b[ k + (i *16)] >= 0x00 && b[ k + (i *16)] <= 0x1F) || (b[ k + (i *16)] >= 0x7F && b[ k + (i *16)] <= 0xFF))
				printf(".");
			else
				printf("%c", b[ k + (i *16)]);
		}
		 printf("|\n");
	}
		
	printf("-------------------------------------------------------------------\n");
	
	return 0;
}
/***********************************************************************
*Funktion zum positionieren des device pointers.
* 	HANDLE device		= device-handle
* 	s64 offset			= byte offset zu dem gesprungen werden soll
***********************************************************************/
s64 seek_device(HANDLE device, s64 byte_offset)
{
	LARGE_INTEGER byte_off;  
	byte_off.QuadPart = byte_offset;
	byte_off.LowPart = SetFilePointer(device, byte_off.LowPart, &byte_off.HighPart, FILE_BEGIN);
	return byte_off.QuadPart;
}
/***********************************************************************
* 1234567890 -> 1.234.567.890
***********************************************************************/
void print_commas(s64 n, char *out)
{
    int c;
    char buf[20];
    char *p;
		
    sprintf(buf, "%lld", n);
    c = 2 - strlen(buf) % 3;
    for (p = buf; *p != 0; p++) {
       *out++ = *p;
       if (c == 1) {
           *out++ = '.';
       }
       c = (c + 1) % 3;
    }
    *--out = 0;
}
