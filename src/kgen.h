/*
* Copyright (c) 2012 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _KGEN_H_
#define _KGEN_H_

#include <stdio.h>
#include "types.h"

void generate_ata_keys(u8 *eid_root_key, u8 *eid_root_iv, u8 *data_key_dst, u8 *tweak_key_dst);
void generate_encdec_keys(u8 *eid_root_key, u8 *eid_root_iv, u8 *data_key_dst, u8 *tweak_key_dst);

#endif
