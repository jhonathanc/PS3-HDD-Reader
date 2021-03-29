/*- 
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fs.h	8.13 (Berkeley) 3/21/95
 * $FreeBSD: /repoman/r/ncvs/src/sys/ufs/ffs/fs.h,v 1.48 2005/02/20 08:02:15 delphij Exp $
 */

#ifndef _UFS_FFS_FS_H_
#define _UFS_FFS_FS_H_



/*
 * A filesystem is described by its super-block, which in turn
 * describes the cylinder groups.  The super-block is critical
 * data and is replicated in each cylinder group to protect against
 * catastrophic loss.  This is done at `newfs' time and the critical
 * super-block data does not change, so the copies need not be
 * referenced further unless disaster strikes.
 *
 * For filesystem fs, the offsets of the various blocks of interest
 * are given in the super block as:
 *	[fs->fs_sblkno]		Super-block
 *	[fs->fs_cblkno]		Cylinder group block
 *	[fs->fs_iblkno]		Inode blocks
 *	[fs->fs_dblkno]		Data blocks
 * The beginning of cylinder group cg in fs, is given by
 * the ``cgbase(fs, cg)'' macro.
 * 
 * Depending on the architecture and the media, the superblock may
 * reside in any one of four places. For tiny media where every block 
 * counts, it is placed at the very front of the partition. Historically,
 * UFS1 placed it 8K from the front to leave room for the disk label and
 * a small bootstrap. For UFS2 it got moved to 64K from the front to leave
 * room for the disk label and a bigger bootstrap, and for really piggy
 * systems we check at 256K from the front if the first three fail. In
 * all cases the size of the superblock will be SBLOCKSIZE. All values are
 * given in byte-offset form, so they do not imply a sector size. The
 * SBLOCKSEARCH specifies the order in which the locations should be searched.
 * 
 */
#define SBLOCK_FLOPPY	    0
#define SBLOCK_UFS1	  		8192				//sector 16
#define SBLOCK_UFS2	 			65536				//sector 128
#define SBLOCK_PIGGY			262144			//sector 512
#define SBLOCKSIZE	  		8192				// 8 kb
#define SBLOCKSEARCH \
	{ SBLOCK_UFS2, SBLOCK_UFS1, SBLOCK_FLOPPY, SBLOCK_PIGGY, -1 }

/*
 * Max number of fragments per block. This value is NOT tweakable.
 * Maximale Anzahl von Fragmenten per Block. Dieser Wert ist nicht änderbar.
 */
#define MAXFRAG 	8

/*
 * Addresses stored in inodes are capable of addressing fragments
 * of `blocks'. File system blocks of at most size MAXBSIZE can
 * be optionally broken into 2, 4, or 8 pieces, each of which is
 * addressable; these pieces may be DEV_BSIZE, or some multiple of
 * a DEV_BSIZE unit.
 * 
 * Large files consist of exclusively large data blocks.  To avoid
 * undue wasted disk space, the last data block of a small file may be
 * allocated as only as many fragments of a large block as are
 * necessary.  The filesystem format retains only a single pointer
 * to such a fragment, which is a piece of a single large block that
 * has been divided.  The size of such a fragment is determinable from
 * information in the inode, using the ``blksize(fs, ip, lbn)'' macro.
 *
 * The filesystem records space availability at the fragment level;
 * to determine block availability, aligned fragments are examined.
 */

/*
 * MINBSIZE is the smallest allowable block size.
 * In order to insure that it is possible to create files of size
 * 2^32 with only two levels of indirection, MINBSIZE is set to 4096.
 * MINBSIZE must be big enough to hold a cylinder group block,
 * thus changes to (struct cg) must keep its size within MINBSIZE.
 * Note that super blocks are always of size SBSIZE,
 * and that both SBSIZE and MAXBSIZE must be >= MINBSIZE.
 * 
 * MINBSIZE ist die kleinste erlaubte block größe. 
 */
#define MINBSIZE	4096 

/*
 * The path name on which the filesystem is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in
 * the super block for this name.
 */
#define MAXMNTLEN	468

/*
 * The volume name for this filesystem is maintained in fs_volname.
 * MAXVOLLEN defines the length of the buffer allocated.
 */
#define MAXVOLLEN	32

/*
 * There is a 128-byte region in the superblock reserved for in-core
 * pointers to summary information. Originally this included an array
 * of pointers to blocks of struct csum; now there are just a few
 * pointers and the remaining space is padded with fs_ocsp[].
 *
 * NOCSPTRS determines the size of this padding. One pointer (fs_csp)
 * is taken away to point to a contiguous array of struct csum for
 * all cylinder groups; a second (fs_maxcluster) points to an array
 * of cluster sizes that is computed as cylinder groups are inspected,
 * and the third points to an array that tracks the creation of new
 * directories. A fourth pointer, fs_active, is used when creating
 * snapshots; it points to a bitmap of cylinder groups for which the
 * free-block bitmap has changed since the snapshot operation began.
 */
#define	NOCSPTRS	((128 / sizeof(void *)) - 4)

/*
 * A summary of contiguous blocks of various sizes is maintained
 * in each cylinder group. Normally this is set by the initial
 * value of fs_maxcontig. To conserve space, a maximum summary size
 * is set by FS_MAXCONTIG.
 */
#define FS_MAXCONTIG	16

/*
 * MINFREE gives the minimum acceptable percentage of filesystem
 * blocks which may be free. If the freelist drops below this level
 * only the superuser may continue to allocate blocks. This may
 * be set to 0 if no reserve of free blocks is deemed necessary,
 * however throughput drops by fifty percent if the filesystem
 * is run at between 95% and 100% full; thus the minimum default
 * value of fs_minfree is 5%. However, to get good clustering
 * performance, 10% is a better choice. hence we use 10% as our
 * default value. With 10% free space, fragmentation is not a
 * problem, so we choose to optimize for time.
 */
#define MINFREE		8
#define DEFAULTOPT	FS_OPTTIME

/*
 * Grigoriy Orlov <gluk@ptci.ru> has done some extensive work to fine
 * tune the layout preferences for directories within a filesystem.
 * His algorithm can be tuned by adjusting the following parameters
 * which tell the system the average file size and the average number
 * of files per directory. These defaults are well selected for typical
 * filesystems, but may need to be tuned for odd cases like filesystems
 * being used for squid caches or news spools.
 */
#define AVFILESIZ	16384	/* expected average file size */
#define AFPDIR		64	/* expected number of files per directory */

/*
 * The maximum number of snapshot nodes that can be associated
 * with each filesystem. This limit affects only the number of
 * snapshot files that can be recorded within the superblock so
 * that they can be found when the filesystem is mounted. However,
 * maintaining too many will slow the filesystem performance, so
 * having this limit is a good idea.
 */
#define FSMAXSNAP 20

/*
 * Used to identify special blocks in snapshots:
 *
 * BLK_NOCOPY - A block that was unallocated at the time the snapshot
 *	was taken, hence does not need to be copied when written.
 * BLK_SNAP - A block held by another snapshot that is not needed by this
 *	snapshot. When the other snapshot is freed, the BLK_SNAP entries
 *	are converted to BLK_NOCOPY. These are needed to allow fsck to
 *	identify blocks that are in use by other snapshots (which are
 *	expunged from this snapshot).
 */
#define BLK_NOCOPY ((ufs2_daddr_t)(1))
#define BLK_SNAP ((ufs2_daddr_t)(2))

/*
 * Sysctl values for the fast filesystem.
 */
#define	FFS_ADJ_REFCNT		 	1		/* adjust inode reference count */
#define	FFS_ADJ_BLKCNT		 	2		/* adjust inode used block count */
#define	FFS_BLK_FREE		 		3		/* free range of blocks in map */
#define	FFS_DIR_FREE		 		4		/* free specified dir inodes in map */
#define	FFS_FILE_FREE		 		5		/* free specified file inodes in map */
#define	FFS_SET_FLAGS		 		6		/* set filesystem flags */
#define	FFS_ADJ_NDIR		    7		/* adjust number of directories */
#define	FFS_ADJ_NBFREE		  8		/* adjust number of free blocks */
#define	FFS_ADJ_NIFREE		  9		/* adjust number of free inodes */
#define	FFS_ADJ_NFFREE		  10 	/* adjust number of free frags */
#define	FFS_ADJ_NUMCLUSTERS	11	/* adjust number of free clusters */
#define	FFS_MAXID						12	/* number of valid ffs ids */

/*
 * Command structure passed in to the filesystem to adjust filesystem values.
 */
#define	FFS_CMD_VERSION		0x19790518	/* version ID */
struct fsck_cmd {
	s32	version;	/* version of command structure */
	s32	handle;		/* reference to filesystem to be changed */
	s64	value;		/* inode or block number to be affected */
	s64	size;			/* amount or range to be adjusted */
	s64	spare;		/* reserved for future use */
};

/*
 * Per cylinder group information; summarized in blocks allocated
 * from first cylinder group data blocks.  These blocks have to be
 * read in from fs_csaddr (size fs_cssize) in addition to the
 * super block.
 */
struct csum {
	s32	cs_ndir;			/* number of directories */
	s32	cs_nbfree;		/* number of free blocks */
	s32	cs_nifree;		/* number of free inodes */
	s32	cs_nffree;		/* number of free frags */
};
struct csum_total {
	s64	cs_ndir;				/* number of directories */
	s64	cs_nbfree;			/* number of free blocks */
	s64	cs_nifree;			/* number of free inodes */
	s64	cs_nffree;			/* number of free frags */
	s64	cs_numclusters;	/* number of free clusters */
	s64	cs_spare[3];		/* future expansion */
};

/*
 * Super block for an FFS filesystem.
 */
struct fs {
	s32	 fs_firstfield;						  // 0x0000: historic filesystem linked list
	s32	 fs_unused_1;							  // 0x0004: used for incore super blocks
	s32	 fs_sblkno;								  // 0x0008: offset of super-block in filesys
	s32	 fs_cblkno;								  // 0x000C: offset of cyl-block in filesys
	s32	 fs_iblkno;								  // 0x0010: offset of inode-blocks in filesys
	s32	 fs_dblkno;								  // 0x0014: offset of first data after cg
	s32	 fs_old_cgoffset;					  // 0x0018: cylinder group offset in cylinder
	s32	 fs_old_cgmask;						  // 0x001C: used to calc mod fs_ntrak
	s32  fs_old_time;							  // 0x0020: last time written
	s32	 fs_old_size;							  // 0x0024: number of blocks in fs
	s32	 fs_old_dsize;						  // 0x0028: number of data blocks in fs
	s32	 fs_ncg;									  // 0x002C: number of cylinder groups
	s32	 fs_bsize;								  // 0x0030: size of basic blocks in fs
	s32	 fs_fsize;								  // 0x0034: size of frag blocks in fs
	s32	 fs_frag;									  // 0x0038: number of frags in a block in fs
	//--------------------------------------------------------------------
  // these are configuration parameters
	s32	 fs_minfree;							  // 0x003C: minimum percentage of free blocks
	s32	 fs_old_rotdelay;					  // 0x0040: num of ms for optimal next block
	s32	 fs_old_rps;						  	// 0x0044: disk revolutions per second
	//--------------------------------------------------------------------
  // these fields can be computed from the others
	s32	 fs_bmask;								  // 0x0048: ``blkoff'' calc of blk offsets
	s32	 fs_fmask;								  // 0x004C: ``fragoff'' calc of frag offsets
	s32	 fs_bshift;								  // 0x0050: ``lblkno'' calc of logical blkno
	s32	 fs_fshift;								  // 0x0054: ``numfrags'' calc number of frags
	//--------------------------------------------------------------------
  // these are configuration parameters
	s32	 fs_maxcontig;						  // 0x0058: max number of contiguous blks
	s32	 fs_maxbpg;								  // 0x005C: max number of blks per cyl group
	//--------------------------------------------------------------------
  // these fields can be computed from the others
	s32	 fs_fragshift;		          // 0x0060: block to frag shift
	s32	 fs_fsbtodb;		            // 0x0064: fsbtodb and dbtofsb shift constant
	s32	 fs_sbsize;		              // 0x0068: actual size of super block
	s32	 fs_spare1[2];		          // 0x006C: old fs_csmask
	//--------------------------------------------------------------------
  // old fs_csshift
	s32	 fs_nindir;		              // 0x0074: value of NINDIR
	s32	 fs_inopb;		              // 0x0078: value of INOPB
	s32	 fs_old_nspf;		            // 0x007C: value of NSPF
	//--------------------------------------------------------------------
  // yet another configuration parameter
	s32	 fs_optim;		              // 0x0080: optimization preference, see below
	s32	 fs_old_npsect;		          // 0x0084: # sectors/track including spares
	s32	 fs_old_interleave;	        // 0x0088: hardware sector interleave
	s32	 fs_old_trackskew;	        // 0x008C: sector 0 skew, per track
	s32	 fs_id[2];		              // 0x0090: unique filesystem id
	//--------------------------------------------------------------------
  // sizes determined by number of cylinder groups and their sizes
	s32	 fs_old_csaddr;		       	  // 0x0098: blk addr of cyl grp summary area
	s32	 fs_cssize;		             	// 0x009C: size of cyl grp summary area
	s32	 fs_cgsize;		             	// 0x00A0: cylinder group size
	s32	 fs_spare2;		           	  // 0x00A4: old fs_ntrak
	s32	 fs_old_nsect;		       	  // 0x00A8: sectors per track
	s32  fs_old_spc;		           	// 0x00AC: sectors per cylinder
	s32	 fs_old_ncyl;		         	  // 0x00B0: cylinders in filesystem
	s32	 fs_old_cpg;		         	  // 0x00B4: cylinders per group
	s32	 fs_ipg;		             	  // 0x00B8: inodes per group
	s32	 fs_fpg;		               	// 0x00BC: blocks per group * fs_frag
	//--------------------------------------------------------------------
  // this data must be re-computed after crashes
	struct	csum fs_old_cstotal;	  // 0x00C0: cylinder summary information
	//--------------------------------------------------------------------
  // these fields are cleared at mount time
	s8   fs_fmod;									  // 0x00D0: super block modified flag
	s8   fs_clean;								  // 0x00D1: filesystem is clean flag
	s8 	 fs_ronly;								  // 0x00D2: mounted read-only flag
	s8   fs_old_flags;						  // 0x00D3: old FS_ flags
	u8	 fs_fsmnt[MAXMNTLEN];			  // 0x00D4: name mounted on
	u8	 fs_volname[MAXVOLLEN];		  // 0x02A8: volume name
	u64 fs_swuid;								    // 0x02C8: system-wide uid
	s32  fs_pad;									  // 0x02D0: due to alignment of fs_swuid
	//--------------------------------------------------------------------
  // these fields retain the current block allocation info
	s32	 fs_cgrotor;							  // 0x02D4: last cg searched
	void 	*fs_ocsp[NOCSPTRS];			  // 0x02D8: padding; was list of fs_cs buffers
	u8 *fs_contigdirs;						  // 0x0338: (u) # of contig. allocated dirs
	struct	csum *fs_csp;				    // 0x0340: (u) cg summary info buffer
	s32	*fs_maxcluster;						  // 0x0348: (u) max cluster in each cyl group
	u_int	*fs_active;						    // 0x0350: (u) used by snapshots to track fs
	s32	 fs_old_cpc;							  // 0x0358: cyl per cycle in postbl
	s32	 fs_maxbsize;							  // 0x035C: maximum blocking factor permitted
	s64	 fs_sparecon64[17];				  // 0x0360: old rotation block list head
	s64	 fs_sblockloc;						  // 0x03E8: byte offset of standard superblock
	struct	csum_total fs_cstotal;  // 0x03F0: (u) cylinder summary information
	ufs_time_t fs_time;							// 0x0430: last time written
	s64	 fs_size;									  // 0x0438: number of blocks in fs
	s64	 fs_dsize;								  // 0x0440: number of data blocks in fs
	ufs2_daddr_t fs_csaddr;					// 0x0448: blk addr of cyl grp summary area
	s64	 fs_pendingblocks;				  // 0x0450: (u) blocks being freed
	s32	 fs_pendinginodes;				  // 0x0458: (u) inodes being freed
	s32	 fs_snapinum[FSMAXSNAP];	  // 0x045C: list of snapshot inode numbers
	s32	 fs_avgfilesize;					  // 0x04AC: expected average file size
	s32	 fs_avgfpdir;							  // 0x04B0: expected # of files per directory
	s32	 fs_save_cgsize;					  // 0x04B4: save real cg size to use fs_bsize
	s32	 fs_sparecon32[26];				  // 0x04B8: reserved for future constants
	s32  fs_flags;								  // 0x0520: see FS_ flags below
	s32	 fs_contigsumsize;				  // 0x0524: size of cluster summary array
	s32	 fs_maxsymlinklen;				  // 0x0528: max length of an internal symlink
	s32	 fs_old_inodefmt;					  // 0x052C: format of on-disk inodes
	u64 fs_maxfilesize;					    // 0x0530: maximum representable file size
	s64	 fs_qbmask;								  // 0x0538: ~fs_bmask for use with 64-bit size
	s64	 fs_qfmask;								  // 0x0540: ~fs_fmask for use with 64-bit size
	s32	 fs_state;								  // 0x0548: validate fs_clean field
	s32	 fs_old_postblformat;			  // 0x054C: format of positional layout tables
	s32	 fs_old_nrpos;						  // 0x0550: number of rotational positions
	s32	 fs_spare5[2];						  // 0x0554: old fs_postbloff
  // old fs_rotbloff
	s32	 fs_magic;								  // 0x055C: magic number
};


/* Sanity checking. */
#ifdef CTASSERT
CTASSERT(sizeof(struct fs) == 1376);
#endif


/*
 * Filesystem identification
 */
#define	FS_UFS1_MAGIC		0x00011954	/* UFS1 fast filesystem magic number */
#define	FS_UFS2_MAGIC		0x19540119	/* UFS2 fast filesystem magic number */
#define	FS_BAD_MAGIC		0x19960408	/* UFS incomplete newfs magic number */
#define	FS_OKAY					0x7c269d38	/* superblock checksum */
#define FS_42INODEFMT		-1					/* 4.2BSD inode format */
#define FS_44INODEFMT		2						/* 4.4BSD inode format */

/*
 * Preference for optimization.
 */
#define FS_OPTTIME	0	/* minimize allocation time */
#define FS_OPTSPACE	1	/* minimize disk fragmentation */

/*
 * Filesystem flags.
 *
 * The FS_UNCLEAN flag is set by the kernel when the filesystem was
 * mounted with fs_clean set to zero. The FS_DOSOFTDEP flag indicates
 * that the filesystem should be managed by the soft updates code.
 * Note that the FS_NEEDSFSCK flag is set and cleared only by the
 * fsck utility. It is set when background fsck finds an unexpected
 * inconsistency which requires a traditional foreground fsck to be
 * run. Such inconsistencies should only be found after an uncorrectable
 * disk error. A foreground fsck will clear the FS_NEEDSFSCK flag when
 * it has successfully cleaned up the filesystem. The kernel uses this
 * flag to enforce that inconsistent filesystems be mounted read-only.
 * The FS_INDEXDIRS flag when set indicates that the kernel maintains
 * on-disk auxiliary indexes (such as B-trees) for speeding directory
 * accesses. Kernels that do not support auxiliary indicies clear the
 * flag to indicate that the indicies need to be rebuilt (by fsck) before
 * they can be used.
 *
 * FS_ACLS indicates that ACLs are administratively enabled for the
 * file system, so they should be loaded from extended attributes,
 * observed for access control purposes, and be administered by object
 * owners.  FS_MULTILABEL indicates that the TrustedBSD MAC Framework
 * should attempt to back MAC labels into extended attributes on the
 * file system rather than maintain a single mount label for all
 * objects.
 */
#define FS_UNCLEAN    		0x01	/* filesystem not clean at mount */
#define FS_DOSOFTDEP  		0x02	/* filesystem using soft dependencies */
#define FS_NEEDSFSCK  		0x04	/* filesystem needs sync fsck before mount */
#define FS_INDEXDIRS  		0x08	/* kernel supports indexed directories */
#define FS_ACLS       		0x10	/* file system has ACLs enabled */
#define FS_MULTILABEL 		0x20	/* file system is MAC multi-label */
#define FS_FLAGS_UPDATED 	0x80	/* flags have been moved to new location */

/*
 * Macros to access bits in the fs_active array.
 */
#define	ACTIVECGNUM(fs, cg)	((fs)->fs_active[(cg) / (NBBY * sizeof(int))])
#define	ACTIVECGOFF(cg)		(1 << ((cg) % (NBBY * sizeof(int))))
#define	ACTIVESET(fs, cg)	do {					\
	if ((fs)->fs_active)						\
		ACTIVECGNUM((fs), (cg)) |= ACTIVECGOFF((cg));		\
} while (0)
#define	ACTIVECLEAR(fs, cg)	do {					\
	if ((fs)->fs_active)						\
		ACTIVECGNUM((fs), (cg)) &= ~ACTIVECGOFF((cg));		\
} while (0)

/*
 * The size of a cylinder group is calculated by CGSIZE. The maximum size
 * is limited by the fact that cylinder groups are at most one block.
 * Its size is derived from the size of the maps maintained in the
 * cylinder group and the (struct cg) size.
 */
#define CGSIZE(fs) \
    /* base cg */	(sizeof(struct cg) + sizeof(s32) + \
    /* old btotoff */	(fs)->fs_old_cpg * sizeof(s32) + \
    /* old boff */	(fs)->fs_old_cpg * sizeof(u16) + \
    /* inode map */	howmany((fs)->fs_ipg, NBBY) + \
    /* block map */	howmany((fs)->fs_fpg, NBBY) +\
    /* if present */	((fs)->fs_contigsumsize <= 0 ? 0 : \
    /* cluster sum */	(fs)->fs_contigsumsize * sizeof(s32) + \
    /* cluster map */	howmany(fragstoblks(fs, (fs)->fs_fpg), NBBY)))

/*
 * The minimal number of cylinder groups that should be created.
 */
#define MINCYLGRPS	4

/*
 * Convert cylinder group to base address of its global summary info.
 */
#define fs_cs(fs, indx) fs_csp[indx]

/*
 * Cylinder group block for a filesystem.
 */
#define	CG_MAGIC	0x090255
struct cg {
	s32	 cg_firstfield;				/* historic cyl groups linked list */
	s32	 cg_magic;						/* magic number */
	s32  cg_old_time;					/* time last written */
	s32	 cg_cgx;							/* we are the cgx'th cylinder group */
	s16	 cg_old_ncyl;					/* number of cyl's this cg */
	s16  cg_old_niblk;				/* number of inode blocks this cg */
	s32	 cg_ndblk;						/* number of data blocks this cg */
	struct	csum cg_cs;						/* cylinder summary information */
	s32	 cg_rotor;						/* position of last used block */
	s32	 cg_frotor;						/* position of last used frag */
	s32	 cg_irotor;						/* position of last used inode */
	s32	 cg_frsum[MAXFRAG];		/* counts of available frags */
	s32	 cg_old_btotoff;			/* (int32) block totals per cylinder */
	s32	 cg_old_boff;					/* (uint16) free block positions */
	s32	 cg_iusedoff;					/* (uint8) used inode map */
	s32	 cg_freeoff;					/* (uint8) free block map */
	s32	 cg_nextfreeoff;			/* (uint8) next available space */
	s32	 cg_clustersumoff;		/* (uint32) counts of avail clusters */
	s32	 cg_clusteroff;				/* (uint8) free cluster map */
	s32	 cg_nclusterblks;			/* number of clusters this cg */
	s32  cg_niblk;						/* number of inode blocks this cg */
	s32	 cg_initediblk;				/* last initialized inode */
	s32	 cg_sparecon32[3];		/* reserved for future use */
	ufs_time_t cg_time;						/* time last written */
	s64	 cg_sparecon64[3];		/* reserved for future use */
	u8 cg_space[1];					/* space for cylinder group maps */
/* actually longer */
};

/*
 * Macros for access to cylinder group array structures
 * 
 * Macros für zugriff auf cylinder group array structuren
 */
#define cg_chkmagic(cgp) ((cgp)->cg_magic == CG_MAGIC)
#define cg_inosused(cgp) \
    ((u8 *)((u8 *)(cgp) + (cgp)->cg_iusedoff))
#define cg_blksfree(cgp) \
    ((u8 *)((u8 *)(cgp) + (cgp)->cg_freeoff))
#define cg_clustersfree(cgp) \
    ((u8 *)((u8 *)(cgp) + (cgp)->cg_clusteroff))
#define cg_clustersum(cgp) \
    ((s32 *)((uintptr_t)(cgp) + (cgp)->cg_clustersumoff))

/*
 * Turn filesystem block numbers into disk block addresses.
 * This maps filesystem blocks to device size blocks.
 * 
 * Wandelt filesystem block nummern in disk block Adressen um.
 * 
 */
#define	fsbtodb(fs, b)	((daddr_t)(b) << (fs)->fs_fsbtodb)
#define	dbtofsb(fs, b)	((b) >> (fs)->fs_fsbtodb)

/*
 * Cylinder group macros to locate things in cylinder groups.
 * They calc filesystem addresses of cylinder group data structures.
 * 
 * 
 */
#define	cgbase(fs, c)	(((ufs2_daddr_t)(fs)->fs_fpg) * (c))
#define	cgdmin(fs, c)	(cgstart(fs, c) + (fs)->fs_dblkno)	/* 1st data */
#define	cgimin(fs, c)	(cgstart(fs, c) + (fs)->fs_iblkno)	/* inode blk */
#define	cgsblock(fs, c)	(cgstart(fs, c) + (fs)->fs_sblkno)	/* super blk */
#define	cgtod(fs, c)	(cgstart(fs, c) + (fs)->fs_cblkno)	/* cg block */
#define cgstart(fs, c)							\
       ((fs)->fs_magic == FS_UFS2_MAGIC ? cgbase(fs, c) :		\
       (cgbase(fs, c) + (fs)->fs_old_cgoffset * ((c) & ~((fs)->fs_old_cgmask))))

/*
 * Macros for handling inode numbers:
 *     inode number to filesystem block offset.
 *     inode number to cylinder group number.
 *     inode number to filesystem block address.
 */
#define	ino_to_cg(fs, x)	((x) / (fs)->fs_ipg)
#define	ino_to_fsba(fs, x)						\
	((ufs2_daddr_t)(cgimin(fs, ino_to_cg(fs, x)) +			\
	    (blkstofrags((fs), (((x) % (fs)->fs_ipg) / INOPB(fs))))))
#define	ino_to_fsbo(fs, x)	((x) % INOPB(fs))

/*
 * Give cylinder group number for a filesystem block.
 * Give cylinder group block number for a filesystem block.
 */
#define	dtog(fs, d)	((d) / (fs)->fs_fpg)
#define	dtogd(fs, d)	((d) % (fs)->fs_fpg)

/*
 * Extract the bits for a block from a map.
 * Compute the cylinder and rotational position of a cyl block addr.
 */
#define blkmap(fs, map, loc) \
    (((map)[(loc) / NBBY] >> ((loc) % NBBY)) & (0xff >> (NBBY - (fs)->fs_frag)))

/*
 * The following macros optimize certain frequently calculated
 * quantities by using shifts and masks in place of divisions
 * modulos and multiplications.
 */
#define blkoff(fs, loc)		/* calculates (loc % fs->fs_bsize) */ \
	((loc) & (fs)->fs_qbmask)
#define fragoff(fs, loc)	/* calculates (loc % fs->fs_fsize) */ \
	((loc) & (fs)->fs_qfmask)
#define lfragtosize(fs, frag)	/* calculates ((off_t)frag * fs->fs_fsize) */ \
	(((off_t)(frag)) << (fs)->fs_fshift)
#define lblktosize(fs, blk)	/* calculates ((off_t)blk * fs->fs_bsize) */ \
	(((off_t)(blk)) << (fs)->fs_bshift)
/* Use this only when `blk' is known to be small, e.g., < NDADDR. */
#define smalllblktosize(fs, blk)    /* calculates (blk * fs->fs_bsize) */ \
	((blk) << (fs)->fs_bshift)
#define lblkno(fs, loc)		/* calculates (loc / fs->fs_bsize) */ \
	((loc) >> (fs)->fs_bshift)
#define numfrags(fs, loc)	/* calculates (loc / fs->fs_fsize) */ \
	((loc) >> (fs)->fs_fshift)
#define blkroundup(fs, size)	/* calculates roundup(size, fs->fs_bsize) */ \
	(((size) + (fs)->fs_qbmask) & (fs)->fs_bmask)
#define fragroundup(fs, size)	/* calculates roundup(size, fs->fs_fsize) */ \
	(((size) + (fs)->fs_qfmask) & (fs)->fs_fmask)
#define fragstoblks(fs, frags)	/* calculates (frags / fs->fs_frag) */ \
	((frags) >> (fs)->fs_fragshift)
#define blkstofrags(fs, blks)	/* calculates (blks * fs->fs_frag) */ \
	((blks) << (fs)->fs_fragshift)
#define fragnum(fs, fsb)	/* calculates (fsb % fs->fs_frag) */ \
	((fsb) & ((fs)->fs_frag - 1))
#define blknum(fs, fsb)		/* calculates rounddown(fsb, fs->fs_frag) */ \
	((fsb) &~ ((fs)->fs_frag - 1))

/*
 * Determine the number of available frags given a
 * percentage to hold in reserve.
 */
#define freespace(fs, percentreserved) \
	(blkstofrags((fs), (fs)->fs_cstotal.cs_nbfree) + \
	(fs)->fs_cstotal.cs_nffree - \
	(((off_t)((fs)->fs_dsize)) * (percentreserved) / 100))

/*
 * Determining the size of a file block in the filesystem.
 */
#define blksize(fs, ip, lbn) \
	(((lbn) >= NDADDR || (ip)->i_size >= smalllblktosize(fs, (lbn) + 1)) \
	    ? (fs)->fs_bsize \
	    : (fragroundup(fs, blkoff(fs, (ip)->i_size))))
#define sblksize(fs, size, lbn) \
	(((lbn) >= NDADDR || (size) >= ((lbn) + 1) << (fs)->fs_bshift) \
	  ? (fs)->fs_bsize \
	  : (fragroundup(fs, blkoff(fs, (size)))))


/*
 * Number of inodes in a secondary storage block/fragment.
 */
#define	INOPB(fs)	((fs)->fs_inopb)
#define	INOPF(fs)	((fs)->fs_inopb >> (fs)->fs_fragshift)

/*
 * Number of indirects in a filesystem block.
 */
#define	NINDIR(fs)	((fs)->fs_nindir)

extern int inside[], around[];
extern u8 *fragtbl[];

#endif
