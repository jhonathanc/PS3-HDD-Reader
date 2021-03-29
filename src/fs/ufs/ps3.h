#ifndef _PS3_H_
#define	_PS3_H_

#define MAX_ACL_ENTRIES		8
#define MAX_PARTITIONS		8

#define MAGIC1						0x0FACE0FFULL
#define MAGIC2						0xDEADFACEULL

struct p_acl_entry {
	u64 laid;
	u64 rights;
};

struct d_partition {
	u64 p_start;
	u64 p_size;
	struct p_acl_entry p_acl[MAX_ACL_ENTRIES];
};

struct disklabel {
	u8 d_res1[16];
	u64 d_magic1;
	u64 d_magic2;
	u64 d_res2;
	u64 d_res3;
	struct d_partition d_partitions[MAX_PARTITIONS];
	u8 d_pad[0x600 - MAX_PARTITIONS * sizeof(struct d_partition)- 0x30];
};


#endif /* _PS3_H_ */
