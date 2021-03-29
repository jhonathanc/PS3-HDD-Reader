#ifndef _FATFS_H_
#define	_FATFS_H_



/* types */
typedef	u32	fat_add_t;    /* cluster adresse */


/* Attribute */
#define	N_LAST				 0x00  /* empty entry, end of dir-entries */
#define	N_DELET				 0xE5  /* entry deleted */
#define	N_ASCII				 0x05  /* first byte is ASCII char 0xE5 */

#define	A_READONLY		 0x01	/* readonly */
#define	A_HIDDEN			 0x02	/* hidden */
#define	A_SYSTEM			 0x04	/* system file */
#define	A_VOLUME			 0x08	/* volume label */
#define	A_L_NAME			 0x0F	/* long filename entry */
#define	A_DIR     		 0x10	/* directory name */
#define	A_FILE  			 0x20	/* archive */

/* ROOT dir marker */
#define	A_ROOT  			 0x80

#define M_LFN_ENTRIES  0x14		/* Max Longfilename entries, 20 */
#define M_LFN_LENGTH   0x0D		/* Max Longfilename entry name lenght, 13 */
#define M_NAME_LENGTH  0x0104	/* Max name lenght */
#define ENTRY_SIZE     0x20		/* Size of one entry, SFN or LFN */

#define FAT32_INFO_SIG_1 0x41615252  /* FAT32 FS info signature 1 */
#define FAT32_INFO_SIG_2 0x61417272  /* FAT32 FS info signature 2 */
#define FAT32_SEC_SIG    0xAA550000  /* FAT32 sector signature */

struct f_entry{
	u32 entry;      /* FAT entry */
}__attribute__ ((__packed__));

typedef struct _fat_clu_list_{
	u32 count;			/* cluster count */
	u32 *clu_add;		/* addres of each cluster */
}fat_clu_list;


#define SECS_PER_MIN    60
#define SECS_PER_HOUR   (60 * 60)
#define SECS_PER_DAY    (SECS_PER_HOUR * 24)
#define UNIX_SECS_1980  315532800L

#if BITS_PER_LONG == 64
#define UNIX_SECS_2108  4354819200L
#endif

/* days between 1.1.70 and 1.1.80 (2 leap days) */
#define DAYS_DELTA      (365 * 10 + 2)

/* 120 (2100 - 1980) isn't leap year */
#define YEAR_2100       120
#define IS_LEAP_YEAR(y) (!((y) & 3) && (y) != YEAR_2100)


/* date time */
struct date_time{
	u8 day;	
	u8 month;	
	u8 year;	
	u8 hour;	
	u8 minutes;	
	u8 seconds;	
}__attribute__ ((__packed__));



/* FAT12/16 Bootsector */
struct fat_bs{
	u8    bs_jc[3];				 	  /* 0x000: Jump Code */
	u8    bs_oem[8];					/* 0x003: OEM ID (ASCII) */
	u16   bs_ssize;				 		/* 0x00B: Bytes Per Sector */
	u8    bs_csize;					 	/* 0x00D: Sectors Per Cluster */
	u16   bs_rsec;            /* 0x00E: No of Reserved Sectors */
	u8    bs_nrfat;          	/* 0x010: No of FATs */
	u16   bs_maxroot;     		/* 0x011: (FAT12/16) Max Root Directory Entries, for FAT32 0 */
	u16   bs_tsec;     		    /* 0x013: Total Sectors in Partition, if Sectors > 65535 = 0 and "No of Sectors in a Partition" is used */
	u8    bs_mtype;           /* 0x015: Media Type, 0xF8 = hdd, 0xF0 for removable */
	u16   bs_fsize;        		/* 0x016: (FAT12/16) Sectors Per FAT, for FAT32 0 */
	u16   bs_tsize;      		  /* 0x018: Sectors Per Track of device */
	u16   bs_nrhead;         	/* 0x01A: No of Heads in device */
	u32   bs_hsec;         		/* 0x01C: No of Sectors before Partition */
	u32   bs_nrsec;      		  /* 0x020: No of Sectors in a Partition, if Sectors > 65535 */
	u8    bs_ldnr;         		/* 0x024: Logical Drive No (BIOS INT 13h) */
	u8    bs_headnr;          /* 0x025: Head No */
	u8    bs_eboot;     		  /* 0x026: Extended Boot Signature, Always 0x29, to identify if the next three values are valid */
	u32   bs_dserial;         /* 0x027: Serial No of the Device */
	u8  	bs_pname[11];       /* 0x02B: Partition Name (ASCII) */
	u8  	bs_ptype[8];        /* 0x036: FAT type (ASCII) */
	u8    bs_bootcode[448];  	/* 0x03E: Executable Boot Code */
	u16   bs_sig;          		/* 0x1FE: Executable Signature (0xAA55) */
}__attribute__ ((__packed__));

/* FAT32 Bootsector */
struct fat32_bs{
  u8    bs_jc[3];				 	  /* 0x000: Jump Code */
	u8    bs_oem[8];					/* OEM ID (ASCII) */
	u16   bs_ssize;				 		/* Bytes Per Sector */
	u8    bs_csize;					 	/* Sectors Per Cluster */
	u16   bs_rsec;            /* No of Reserved Sectors */
	u8    bs_nrfat;          	/* No of FATs */
	u16   bs_maxroot;     		/* (FAT12/16) Max Root Directory Entries, for FAT32 0 */
	u16   bs_tsec;     		    /* Total Sectors in Partition, if Sectors > 65535 = 0 and "No of Sectors in a Partition" is used */
	u8    bs_mtype;           /* Media Type, 0xF8 = hdd, 0xF0 for removable */
	u16   bs_fsize;        		/* (FAT12/16) Sectors Per FAT, for FAT32 0 */
	u16   bs_tsize;      		  /* Sectors Per Track of device */
	u16   bs_nrhead;         	/* No of Heads in device */
	u32   bs_hsec;         		/* No of Sectors before Partition */
	u32   bs_nrsec;      		  /* No of Sectors in a Partition, if Sectors > 65535 */
	u32   bs32_fsize;         /* Sectors Per FAT */
	u16   bs32_nrfat;         /* FAT structures */
	u16   bs32_mmvnr;         /* major and minor version number */
	u32   bs32_rootclu;       /* root directory Cluster */
	u16   bs32_fsinfo;        /* Sector for FSINFO */
	u16   bs32_bsbackup;      /* Sector for backup copy of boot sector */
	u8    bs32_res1[12];      /* Reserved */
	u8    bs32_ldnr;         	/* Logical Drive No (BIOS INT 13h) */
	u8    bs32_res2;          /* Reserved */
	u8    bs32_eboot;     		/* Extended Boot Signature, Always 0x29, to identify if the next three values are valid */
	u32   bs32_dserial;       /* Serial No of the Device */
	u8  	bs32_pname[11];     /* Partition Name (ASCII) */
	u8  	bs32_ptype[8];      /* FAT type (ASCII) */
	u8    bs32_bootcode[420]; /* Executable Boot Code */
	u16   bs32_sig;         	/* Executable Signature (0xAA55) */
}__attribute__ ((__packed__));

/* FAT12/16 Infosector */
struct fat32_info{
	u32   i_m1;          /* FAT32 FS info signature 1 (0x41615252) */
	u8    i_res1[480];   /* Reserved */
	u32   i_m2;          /* FAT32 FS info signature 2 (0x61417272) */
	u32   i_fclu;        /* Number of free clusters */
	u32   i_nfclu;       /* Next free cluster */
	u8    i_res2[12];    /* Reserved */
	u32   i_sig;         /* FAT32 sector signature (0xAA550000) */
}__attribute__ ((__packed__));

struct sfn_e{
	u8    d_name[11];				 	/* name, is first byte 0xE5 or 0x00 = unallocated */
	u8		d_att;							/* Attribute */
	u8		d_case;							/* Case */
	u8		d_ctime_ms;         /* Creation time in ms,  10tel Sekunde (0 - 199) */
	u16		d_ctime;            /* Creation time */
	u16		d_cdate;            /* Creation date */
	u16		d_adate;            /* Last access date */
	u16		d_atime;            /* Last access time */
	u16   d_mtime;						/* Last modified time */
	u16   d_mdate;						/* Last modified date */
	u16   d_start_clu;				/* Starting cluster */
	u32		d_size;							/* Size of the file */
}__attribute__ ((__packed__));

struct lfn_e{
	u8  ld_ord;         /* Sequence number (XOR'ed with 0x40), and allocation status (0xe5 if unallocated) */
	u8  ld_fname1[10];  /* File name characters 1–5 (Unicode) */
	u8	ld_att;			    /* Attribute, for lnf 0x0F */
	u8  ld_res1;        /* Reserved */
	u8  ld_csum;        /* Checksum */
	u8  ld_fname2[12];  /* File name characters 6–11 (Unicode) */
	u16 ld_res2;        /* Reserved */
	u8  ld_fname3[4];   /* File name characters 12–13 (Unicode) */
}__attribute__ ((__packed__));

struct fat_dir_entry{
	u8  fd_dos_name[11];						/* name, is first byte 0xE5 or 0x00 = unallocated */
	u8	fd_att;											/* Attribute */
	u8	fd_case;										/* Case */
	u8	fd_ctime_ms;         				/* Creation time in ms,  10tel Sekunde (0 - 199) */
	u16	fd_ctime;            				/* Creation time */
	u16	fd_cdate;            				/* Creation date */
	u16	fd_atime;            				/* Last access time */
	u16 fd_adate;          				  /* Last access date */
	u16 fd_mtime;									  /* Last modified time */
	u16 fd_mdate;									  /* Last modified date */
	u16 fd_start_clu;								/* Starting cluster */
	u32	fd_size;										/* Size of the file */
	u8 	fd_name[M_NAME_LENGTH + 1];	/* name of entry */
}__attribute__ ((__packed__));


/* Macros FAT12/16*/
/* berechne cluster size. */
#define clu_size(fat_bs) (((fat_bs)->bs_csize) * ((fat_bs)->bs_ssize))
/* berechne root-dir size. */
#define root_size(fat_bs) (((fat_bs)->bs_maxroot) * (ENTRY_SIZE))
/* berechne offset von fat 1. */
#define fat1_off(fat_bs) (((fat_bs)->bs_rsec) * ((fat_bs)->bs_ssize))
/* berechne size einer fat. */
#define fat_size(fat_bs) (((fat_bs)->bs_fsize) * ((fat_bs)->bs_ssize))
/* berechne root-cluster offset adresse. */
#define root_off(fat_bs) ((fat1_off(fat_bs)) + ((fat_size(fat_bs)) * ((fat_bs)->bs_nrfat)))
/* berechne offset adresse für data area. */
#define data_off(fat_bs) ((fat1_off(fat_bs)) + ((fat_size(fat_bs)) * \
((fat_bs)->bs_nrfat)) + (root_size(fat_bs)))
/* berechne offset adresse für einen cluster. */
#define clu_off(fat_bs, c) (data_off(fat_bs) + (clu_size(fat_bs) * ((c) - (2))))
/* root dir cluster count */
#define root_clu(fat_bs) (root_size(fat_bs) / clu_size(fat_bs))

/* Macros FAT32*/
/* berechne cluster size. */
#define clu_size32(fat32_bs) (((fat32_bs)->bs_csize) * ((fat32_bs)->bs_ssize))
/* berechne offset von fat 1. */
#define fat1_off32(fat32_bs) (((fat32_bs)->bs_rsec) * ((fat32_bs)->bs_ssize))
/* berechne size einer fat. */
#define fat_size32(fat32_bs) (((fat32_bs)->bs32_fsize) * ((fat32_bs)->bs_ssize))
/* berechne offset adresse für data area. */

#define data_off32(fat32_bs) ((fat1_off32(fat32_bs)) + ((fat_size32(fat32_bs)) * \
((fat32_bs)->bs_nrfat)))
/* berechne offset adresse für einen cluster. */
#define clu_off32(fat32_bs, c) (data_off32(fat32_bs) + (clu_size32(fat32_bs) * ((c) - (2))))

/* berechne offset eines FAT12-entrys */
#define fat12_e(fat_bs, c) (fat1_off(fat_bs) + ((c) * (3) / (2)))
/* berechne offset eines FAT16-entrys */
#define fat16_e(fat_bs, c) (fat1_off(fat_bs) + ((c) * (2)))
/* berechne offset eines FAT16-entrys */
#define fat32_e(fat32_bs, c) (fat1_off32(fat32_bs) + ((c) * (4)))


#endif /* _FATFS_H_ */
