#ifndef _NVRAM_H
#define _NVRAM_H 	1

//MUST be same as /proc/mtd
#define FLASH_BOOTLOADER_CONFIG		"bootloader-config"
#define	FLASH_PRIMARY				"config1"
#define	FLASH_SECONDARY				"config2"
//MUST be equal or less than MTD real size
#define	FLASH_SECTOR_SIZE			(0x10000)

typedef struct flash_t {
	long crc;	/* CRC32 over data bytes    */
	char data[FLASH_SECTOR_SIZE - sizeof(long)];
} flash_s;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

//-----------------------------------------------------------------------
//from linux/include/mtd/mtd-abi.h
//-----------------------------------------------------------------------
struct mtd_info_user {
	uint8_t type;
	uint32_t flags;
	uint32_t size;		// Total size of the MTD
	uint32_t erasesize;
	uint32_t oobblock;	// Size of OOB blocks (e.g. 512)
	uint32_t oobsize;	// Amount of OOB data per block (e.g. 16)
	uint32_t ecctype;
	uint32_t eccsize;
};

struct region_info_user {
	uint32_t offset;	/* At which this region starts,
				 * from the beginning of the MTD */
	uint32_t erasesize;	/* For this region */
	uint32_t numblocks;	/* Number of blocks in this region */
	uint32_t regionindex;
};

struct otp_info {
	uint32_t start;
	uint32_t length;
	uint32_t locked;
};

struct erase_info_user {
	uint32_t start;
	uint32_t length;
};

struct mtd_oob_buf {
	uint32_t start;
	uint32_t length;
	char *ptr;
};

#define MEMGETINFO              _IOR('M', 1, struct mtd_info_user)
#define MEMERASE                _IOW('M', 2, struct erase_info_user)
#define MEMWRITEOOB             _IOWR('M', 3, struct mtd_oob_buf)
#define MEMREADOOB              _IOWR('M', 4, struct mtd_oob_buf)
#define MEMLOCK                 _IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK               _IOW('M', 6, struct erase_info_user)
#define MEMGETREGIONCOUNT   _IOR('M', 7, int)
#define MEMGETREGIONINFO    _IOWR('M', 8, struct region_info_user)
#define MEMSETOOBSEL        _IOW('M', 9, struct nand_oobinfo)
#define MEMGETOOBSEL        _IOR('M', 10, struct nand_oobinfo)
#define MEMGETBADBLOCK      _IOW('M', 11, loff_t)
#define MEMSETBADBLOCK      _IOW('M', 12, loff_t)
#define OTPSELECT       _IOR('M', 13, int)
#define OTPGETREGIONCOUNT   _IOW('M', 14, int)
#define OTPGETREGIONINFO    _IOW('M', 15, struct otp_info)
#define OTPLOCK     _IOR('M', 16, struct otp_info)
//-----------------------------------------------------------------------

#endif
