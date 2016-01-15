#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//linux include
#include <linux/types.h>

#include "flash.h"

extern void PRINT_MSG(const char *format, ...);
extern unsigned long crc32(unsigned long, char *, unsigned long);

int open_mtd_block(char *mtd_name, int flags)
{
	FILE *fp;
	char dev[80];
	int i, ret;

	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd_name)) {
				snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
				if ((ret = open(dev, flags)) < 0) {
					snprintf(dev, sizeof(dev), "/dev/mtd%d",
						 i);
					ret = open(dev, flags);
				}
				snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
				fclose(fp);
				return ret;
			}
		}
		fclose(fp);
	}
	return -1;
}

//----------------------------------------------------------------------
//this function is standalone, do NOT use within another flash function.
//Use only to get flash size at initial time.
int get_mtd_size(int active)
{
	struct mtd_info_user mtd_info;
	char *fname[64];
	int fd;

	//choice
	if (active == 0)
		strcpy((char *)fname, FLASH_PRIMARY);
	else
		strcpy((char *)fname, FLASH_SECONDARY);

	//open flash
	fd = open_mtd_block((char *)fname, O_RDWR);
	if (fd < 0) {
		PRINT_MSG("can't open flash device.");
		return -1;
	}
	if (ioctl(fd, MEMGETINFO, &mtd_info) != 0) {
		PRINT_MSG("ioctl(MEMGETINFO) error : ");
		close(fd);
		return -1;
	}
	close(fd);

	return mtd_info.size;
}

//----------------------------------------------------------------------

int erase_flash_block(int fd)
{
	struct mtd_info_user mtd_info;
	struct erase_info_user erase_info;

	if (ioctl(fd, MEMGETINFO, &mtd_info) != 0) {
		PRINT_MSG("ioctl(MEMGETINFO) error : ");
		return -1;
	}

	erase_info.length = mtd_info.erasesize;
	for (erase_info.start = 0x0; erase_info.start < mtd_info.size;
	     erase_info.start += mtd_info.erasesize) {
		//if (ioctl(fd, MEMUNLOCK, &erase_info) != 0) {
		if (0) {
			PRINT_MSG("ioctl(MEMUNLOCK) error : ");
			return -1;
		}

		if (ioctl(fd, MEMERASE, &erase_info) != 0) {
			PRINT_MSG("ioctl(MEMERASE) error : ");
			return -1;
		}
	}
	return 0;
}

//active is 0 or 1
//0 = primary
//1 = secondary
int get_lowdata_from_flash(flash_s * flash, int active)
{
	int fd;
	int ret;
	char *fname[64];

	//choice
	if (active == 0)
		strcpy((char *)fname, FLASH_PRIMARY);
	else
		strcpy((char *)fname, FLASH_SECONDARY);

	//open flash
	fd = open_mtd_block((char *)fname, O_RDWR);
	if (fd < 0) {
		PRINT_MSG("can't open flash device.");
		return -1;
	}
	//read CRC32 : ?endian?
	lseek(fd, 0, SEEK_SET);
	ret = read(fd, &(flash->crc), sizeof(unsigned long));
	if (fd < 0) {
		PRINT_MSG("can't open flash device.");
		return -1;
	}
	//read  
	lseek(fd, sizeof(unsigned long), SEEK_SET);
	ret =
	    read(fd, &(flash->data[0]),
		 FLASH_SECTOR_SIZE - sizeof(unsigned long));
	if (ret < 0) {
		PRINT_MSG("can't read flash device.");
		return -1;
	}
	close(fd);

	//verify CRC check
	if (crc32
	    (0, &(flash->data[0]),
	     FLASH_SECTOR_SIZE - sizeof(unsigned long)) != flash->crc) {
		PRINT_MSG("Bad CRC %x, ignore values in flash.", flash->crc);
		//reloading factory ... or secondary..
		return -1;
	}

	return 0;
}

int set_flash_with_lowdata(flash_s * flash, int active)
{
	int fd;
	int ret;
	char *fname[64];

	//choice
	if (active == 0)
		strcpy((char *)fname, FLASH_PRIMARY);
	else
		strcpy((char *)fname, FLASH_SECONDARY);

	//open flash
	fd = open_mtd_block((char *)fname, O_RDWR);
	if (fd < 0) {
		PRINT_MSG("can't open flash device.");
		return -1;
	}
	//erase block
	erase_flash_block(fd);

	//calc CRC and write
	flash->crc =
	    crc32(0, &(flash->data[0]),
		  FLASH_SECTOR_SIZE - sizeof(unsigned long));

	//write buffer  
	lseek(fd, 0, SEEK_SET);
	ret = write(fd, flash, FLASH_SECTOR_SIZE);
	if (ret < 0) {
		PRINT_MSG("can't write flash device.");
		PRINT_MSG("errno = [%d]", errno);
		return -1;
	}
	close(fd);

	return 0;
}

/**********************************************
For Bootloader config
**********************************************/
static int cp_mtd2buf(char *buf)
{
	int fd;
	ssize_t n;

	fd = open_mtd_block(FLASH_BOOTLOADER_CONFIG, O_RDWR);
	if (fd < 0) {
		PRINT_MSG("can't open '%s'\n", FLASH_BOOTLOADER_CONFIG);
		PRINT_MSG("errno: %d\n", errno);
		perror("mtdopen");
		return -1;
	}

	n = read(fd, buf, FLASH_SECTOR_SIZE);
	if (close(fd) < 0) {
		PRINT_MSG("error to cloase '%s'\n", FLASH_BOOTLOADER_CONFIG);
		return -1;
	}
	return n;
}

static int cp_buf2mtd(char *buf)
{
	int fd;
	int n;

	fd = open(FLASH_BOOTLOADER_CONFIG, O_RDWR);
	if (fd < 0) {
		PRINT_MSG("can't open '%s'\n", FLASH_BOOTLOADER_CONFIG);
		PRINT_MSG("errno: %d\n", errno);
		perror("mtdopen");
		return -1;
	}

	lseek(fd, 0, SEEK_SET);
	n = write(fd, buf, FLASH_SECTOR_SIZE);
	PRINT_MSG("write n : %d\n", n);
	if (close(fd) < 0) {
		PRINT_MSG("error to cloase '%s'\n", FLASH_BOOTLOADER_CONFIG);
		return -1;
	}
	return n;
}

/************************************************************************
 * Match a name / name=value pair
 *
 * s1 is either a simple 'name', or a 'name=value' pair.
 * i2 is the environment index for a 'name2=value2' pair.
 * If the names match, return the index for the value2, else NULL.
 */
static int envmatch(flash_s * env, char *s1, int i2)
{
	while (*s1 == env->data[i2++]) {
		if (*s1++ == '=') {
			return (i2);
		}
	}
	if (*s1 == '\0' && env->data[i2 - 1] == '=')
		return (i2);
	return (-1);
}

int get_bootcfg(char *name, char *val)
{
	flash_s bootenv;
	int i, j, k, nxt = 0;

	cp_mtd2buf((char *)&bootenv);

	k = -1;
	for (j = 0; bootenv.data[j] != '\0'; j = nxt + 1) {
		for (nxt = j; bootenv.data[nxt] != '\0'; ++nxt) {
    }

		k = envmatch(&bootenv, name, j);
		if (k < 0)
			continue;

		i = 0;
		while (k < nxt) {
			val[i] = bootenv.data[k++];
			i++;
		}
		val[i] = '\0';
		break;
	}
	if (k < 0) {
		PRINT_MSG("## Error: \"%s\" not defined\n", name);
		return 1;
	}

	return 0;
}

/* This function will ONLY work with a in-RAM copy of the environment */
int set_bootcfg(char *name, char *vals)
{
	flash_s bootenv;
	int len, oldval;
	char *env, *nxt = 0;

	cp_mtd2buf((char *)&bootenv);

	/*
	 * search if variable with this name already exists
	 */
	oldval = -1;
	for (env = bootenv.data; *env; env = nxt + 1) {
		for (nxt = env; *nxt; ++nxt) {
    } 
		if ((oldval =
		     envmatch(&bootenv, name,
			      (unsigned long)env -
			      (unsigned long)bootenv.data)) >= 0) {
			PRINT_MSG("matched env name =[%s]\n", name);
			break;
		}
	}

	/*
	 * Delete any existing definition
	 */
	if (oldval >= 0) {
		if (*++nxt == '\0') {
			if ((unsigned long)env > (unsigned long)bootenv.data) {
				env--;
			} else {
				*env = '\0';
			}
		} else {
			for (;;) {
				*env = *nxt++;
				if ((*env == '\0') && (*nxt == '\0'))
					break;
				++env;
			}
		}
		*++env = '\0';
	}
	/* Delete only ? */
	if (vals == NULL)
		goto out;
	/*
	 * Append new definition at the end
	 */
	for (env = bootenv.data; *env || *(env + 1); ++env) {
  }
	if ((unsigned long)env > (unsigned long)bootenv.data)
		++env;

	len = strlen(name) + 2;

	/* add '=' for first arg, ' ' for all others */
	PRINT_MSG("vals : %s\n", vals);
	len = strlen(vals) + 1;

	if (len > sizeof(bootenv.data)) {
		PRINT_MSG("## Error: environment overflow, \"%s\" deleted\n",
			  name);
		return 1;
	}

	while ((*env = *name++) != '\0') {
		env++;
  }

//  *env = (i==0) ? '=' : ' ';
	*env = '=';
	while ((*++env = *vals++) != '\0') {
	}

	/* end is marked with double '\0' */
	*++env = '\0';
 out:
	PRINT_MSG("env : %s\n", bootenv.data);
	/* Update CRC */
	bootenv.crc = crc32(0, bootenv.data, sizeof(bootenv.data));
	cp_buf2mtd((char *)&bootenv);

	return 0;
}
