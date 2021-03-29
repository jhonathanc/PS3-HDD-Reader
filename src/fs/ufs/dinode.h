#ifndef _UFS_UFS_DINODE_H_
#define	_UFS_UFS_DINODE_H_


#define	ROOTINO	((ino_t)2)

#define	WINO	((ino_t)1)


typedef struct _ufs2_block_list_ {
	s64 *blk_add;
} ufs2_block_list;


typedef	s64	ufs2_daddr_t;
typedef s64 ufs_lbn_t;
typedef s64 ufs_time_t;
typedef s64 ufs_inop;

/* File permissions. */
#define	IEXEC		0000100		/* Executable. */
#define	IWRITE	0000200		/* Writeable. */
#define	IREAD		0000400		/* Readable. */
#define	ISVTX		0001000		/* Sticky bit. */
#define	ISGID		0002000		/* Set-gid. */
#define	ISUID		0004000		/* Set-uid. */

/* File types. */
#define	IFMT		0170000		/* Mask of file type. */
#define	IFIFO		0010000		/* Named pipe (fifo). */
#define	IFCHR		0020000		/* Character device. */
#define	IFDIR		0040000		/* Directory file. */
#define	IFBLK		0060000		/* Block device. */
#define	IFREG		0100000		/* Regular file. */
#define	IFLNK		0120000		/* Symbolic link. */
#define	IFSOCK	0140000		/* UNIX domain socket. */
#define	IFWHT		0160000		/* Whiteout. */


#define	NXADDR	2			/* External addresses in inode. */
#define	NDADDR	12		/* Direct addresses in inode. */
#define	NIADDR	3			/* Indirect addresses in inode. */

struct ufs2_dinode {
	u16	di_mode;	      		/*   0: IFMT, permissions; see below. */ 
	s16		di_nlink;	      		/*   2: File link count. */
	u32	di_uid;		      		/*   4: File owner. */ 
	u32	di_gid;							/*   8: File group. */
	u32	di_blksize;					/*  12: Inode blocksize. */
	u64	di_size;						/*  16: File byte count. */
	u64	di_blocks;					/*  24: Blocks actually held. */
	ufs_time_t	di_atime;					/*  32: Last access time. */
	ufs_time_t	di_mtime;					/*  40: Last modified time. */
	ufs_time_t	di_ctime;					/*  48: Last inode change time. */
	ufs_time_t	di_birthtime;			/*  56: Inode creation time. */
	s32		di_mtimensec;				/*  64: Last modified time. */
	s32		di_atimensec;				/*  68: Last access time. */
	s32		di_ctimensec;				/*  72: Last inode change time. */
	s32		di_birthnsec;				/*  76: Inode creation time. */
	s32		di_gen;							/*  80: Generation number. */
	u32	di_kernflags;				/*  84: Kernel flags. */
	u32	di_flags;						/*  88: Status flags (chflags). */
	s32		di_extsize;					/*  92: External attributes block. */
	ufs2_daddr_t	di_extb[NXADDR];/*  96: External attributes block. */
	ufs2_daddr_t	di_db[NDADDR];	/* 112: Direct disk blocks. */
	ufs2_daddr_t	di_ib[NIADDR];	/* 208: Indirect disk blocks. */
	s64		di_spare[3];				/* 232: Reserved; currently unused */
}__attribute__ ((__packed__));



#endif /* _UFS_UFS_DINODE_H_ */
