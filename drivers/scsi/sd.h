/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SCSI_DISK_H
#define _SCSI_DISK_H

#if defined(CONFIG_UFS_SRPMB)
#include "scsi_srpmb.h"
#endif

/*
 * More than enough for everybody ;)  The huge number of majors
 * is a leftover from 16bit dev_t days, we don't really need that
 * much numberspace.
 */
#define SD_MAJORS	16

/*
 * Time out in seconds for disks and Magneto-opticals (which are slower).
 */
#define SD_TIMEOUT		(30 * HZ)
#define SD_MOD_TIMEOUT		(75 * HZ)
#define SD_UFS_TIMEOUT		(10 * HZ)
#define SD_UFS_FLUSH_TIMEOUT		(6 * HZ)

/*
 * Flush timeout is a multiplier over the standard device timeout which is
 * user modifiable via sysfs but initially set to SD_TIMEOUT
 */
#define SD_FLUSH_TIMEOUT_MULTIPLIER	2
#define SD_WRITE_SAME_TIMEOUT	(120 * HZ)

/*
 * Number of allowed retries
 */
#define SD_MAX_RETRIES		5
#define SD_PASSTHROUGH_RETRIES	1
#define SD_MAX_MEDIUM_TIMEOUTS	2

/*
 * Size of the initial data buffer for mode and read capacity data
 */
#define SD_BUF_SIZE		512

/*
 * Number of sectors at the end of the device to avoid multi-sector
 * accesses to in the case of last_sector_bug
 */
#define SD_LAST_BUGGY_SECTORS	8

enum {
	SD_EXT_CDB_SIZE = 32,	/* Extended CDB size */
	SD_MEMPOOL_SIZE = 2,	/* CDB pool size */
};

enum {
	SD_DEF_XFER_BLOCKS = 0xffff,
	SD_MAX_XFER_BLOCKS = 0xffffffff,
	SD_MAX_WS10_BLOCKS = 0xffff,
	SD_MAX_WS16_BLOCKS = 0x7fffff,
};

enum {
	SD_LBP_FULL = 0,	/* Full logical block provisioning */
	SD_LBP_UNMAP,		/* Use UNMAP command */
	SD_LBP_WS16,		/* Use WRITE SAME(16) with UNMAP bit */
	SD_LBP_WS10,		/* Use WRITE SAME(10) with UNMAP bit */
	SD_LBP_ZERO,		/* Use WRITE SAME(10) with zero payload */
	SD_LBP_DISABLE,		/* Discard disabled due to failed cmd */
};

enum {
	SD_ZERO_WRITE = 0,	/* Use WRITE(10/16) command */
	SD_ZERO_WS,		/* Use WRITE SAME(10/16) command */
	SD_ZERO_WS16_UNMAP,	/* Use WRITE SAME(16) with UNMAP */
	SD_ZERO_WS10_UNMAP,	/* Use WRITE SAME(10) with UNMAP */
};

struct scsi_disk {
	struct scsi_driver *driver;	/* always &sd_template */
	struct scsi_device *device;
	struct device	dev;
	struct gendisk	*disk;
	struct opal_dev *opal_dev;
#ifdef CONFIG_BLK_DEV_ZONED
	u32		nr_zones;
	u32		zone_blocks;
	u32		zone_shift;
	u32		zones_optimal_open;
	u32		zones_optimal_nonseq;
	u32		zones_max_open;
#endif
	atomic_t	openers;
	sector_t	capacity;	/* size in logical blocks */
	u32		max_xfer_blocks;
	u32		opt_xfer_blocks;
	u32		max_ws_blocks;
	u32		max_unmap_blocks;
	u32		unmap_granularity;
	u32		unmap_alignment;
	u32		index;
	unsigned int	physical_block_size;
	unsigned int	max_medium_access_timeouts;
	unsigned int	medium_access_timed_out;
	u8		media_present;
	u8		write_prot;
	u8		protection_type;/* Data Integrity Field */
	u8		provisioning_mode;
	u8		zeroing_mode;
	unsigned	ATO : 1;	/* state of disk ATO bit */
	unsigned	cache_override : 1; /* temp override of WCE,RCD */
	unsigned	WCE : 1;	/* state of disk WCE bit */
	unsigned	RCD : 1;	/* state of disk RCD bit, unused */
	unsigned	DPOFUA : 1;	/* state of disk DPOFUA bit */
	unsigned	first_scan : 1;
	unsigned	lbpme : 1;
	unsigned	lbprz : 1;
	unsigned	lbpu : 1;
	unsigned	lbpws : 1;
	unsigned	lbpws10 : 1;
	unsigned	lbpvpd : 1;
	unsigned	ws10 : 1;
	unsigned	ws16 : 1;
	unsigned	rc_basis: 2;
	unsigned	zoned: 2;
	unsigned	urswrz : 1;
	unsigned	security : 1;
	unsigned	ignore_medium_access_errors : 1;
#ifdef CONFIG_USB_STORAGE_DETECT
	wait_queue_head_t	delay_wait;
	struct completion	scanning_done;
	struct task_struct	*th;
	int		thread_remove;
	int		async_end;
	int		prv_media_present;
#endif

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
};
#define to_scsi_disk(obj) container_of(obj,struct scsi_disk,dev)

static inline struct scsi_disk *scsi_disk(struct gendisk *disk)
{
	return container_of(disk->private_data, struct scsi_disk, driver);
}

#define sd_printk(prefix, sdsk, fmt, a...)				\
        (sdsk)->disk ?							\
	      sdev_prefix_printk(prefix, (sdsk)->device,		\
				 (sdsk)->disk->disk_name, fmt, ##a) :	\
	      sdev_printk(prefix, (sdsk)->device, fmt, ##a)

#define sd_first_printk(prefix, sdsk, fmt, a...)			\
	do {								\
		if ((sdkp)->first_scan)					\
			sd_printk(prefix, sdsk, fmt, ##a);		\
	} while (0)

static inline int scsi_medium_access_command(struct scsi_cmnd *scmd)
{
	switch (scmd->cmnd[0]) {
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
	case SYNCHRONIZE_CACHE:
	case VERIFY:
	case VERIFY_12:
	case VERIFY_16:
	case WRITE_6:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
	case WRITE_SAME:
	case WRITE_SAME_16:
	case UNMAP:
		return 1;
	case VARIABLE_LENGTH_CMD:
		switch (scmd->cmnd[9]) {
		case READ_32:
		case VERIFY_32:
		case WRITE_32:
		case WRITE_SAME_32:
			return 1;
		}
	}

	return 0;
}

static inline sector_t logical_to_sectors(struct scsi_device *sdev, sector_t blocks)
{
	return blocks << (ilog2(sdev->sector_size) - 9);
}

static inline unsigned int logical_to_bytes(struct scsi_device *sdev, sector_t blocks)
{
	return blocks * sdev->sector_size;
}

static inline sector_t bytes_to_logical(struct scsi_device *sdev, unsigned int bytes)
{
	return bytes >> ilog2(sdev->sector_size);
}

static inline sector_t sectors_to_logical(struct scsi_device *sdev, sector_t sector)
{
	return sector >> (ilog2(sdev->sector_size) - 9);
}

/*
 * Look up the DIX operation based on whether the command is read or
 * write and whether dix and dif are enabled.
 */
static inline unsigned int sd_prot_op(bool write, bool dix, bool dif)
{
	/* Lookup table: bit 2 (write), bit 1 (dix), bit 0 (dif) */
	const unsigned int ops[] = {	/* wrt dix dif */
		SCSI_PROT_NORMAL,	/*  0	0   0  */
		SCSI_PROT_READ_STRIP,	/*  0	0   1  */
		SCSI_PROT_READ_INSERT,	/*  0	1   0  */
		SCSI_PROT_READ_PASS,	/*  0	1   1  */
		SCSI_PROT_NORMAL,	/*  1	0   0  */
		SCSI_PROT_WRITE_INSERT, /*  1	0   1  */
		SCSI_PROT_WRITE_STRIP,	/*  1	1   0  */
		SCSI_PROT_WRITE_PASS,	/*  1	1   1  */
	};

	return ops[write << 2 | dix << 1 | dif];
}

/*
 * Returns a mask of the protection flags that are valid for a given DIX
 * operation.
 */
static inline unsigned int sd_prot_flag_mask(unsigned int prot_op)
{
	const unsigned int flag_mask[] = {
		[SCSI_PROT_NORMAL]		= 0,

		[SCSI_PROT_READ_STRIP]		= SCSI_PROT_TRANSFER_PI |
						  SCSI_PROT_GUARD_CHECK |
						  SCSI_PROT_REF_CHECK |
						  SCSI_PROT_REF_INCREMENT,

		[SCSI_PROT_READ_INSERT]		= SCSI_PROT_REF_INCREMENT |
						  SCSI_PROT_IP_CHECKSUM,

		[SCSI_PROT_READ_PASS]		= SCSI_PROT_TRANSFER_PI |
						  SCSI_PROT_GUARD_CHECK |
						  SCSI_PROT_REF_CHECK |
						  SCSI_PROT_REF_INCREMENT |
						  SCSI_PROT_IP_CHECKSUM,

		[SCSI_PROT_WRITE_INSERT]	= SCSI_PROT_TRANSFER_PI |
						  SCSI_PROT_REF_INCREMENT,

		[SCSI_PROT_WRITE_STRIP]		= SCSI_PROT_GUARD_CHECK |
						  SCSI_PROT_REF_CHECK |
						  SCSI_PROT_REF_INCREMENT |
						  SCSI_PROT_IP_CHECKSUM,

		[SCSI_PROT_WRITE_PASS]		= SCSI_PROT_TRANSFER_PI |
						  SCSI_PROT_GUARD_CHECK |
						  SCSI_PROT_REF_CHECK |
						  SCSI_PROT_REF_INCREMENT |
						  SCSI_PROT_IP_CHECKSUM,
	};

	return flag_mask[prot_op];
}

#ifdef CONFIG_BLK_DEV_INTEGRITY

extern void sd_dif_config_host(struct scsi_disk *);

#else /* CONFIG_BLK_DEV_INTEGRITY */

static inline void sd_dif_config_host(struct scsi_disk *disk)
{
}

#endif /* CONFIG_BLK_DEV_INTEGRITY */

static inline int sd_is_zoned(struct scsi_disk *sdkp)
{
	return sdkp->zoned == 1 || sdkp->device->type == TYPE_ZBC;
}

#ifdef CONFIG_BLK_DEV_ZONED

extern int sd_zbc_read_zones(struct scsi_disk *sdkp, unsigned char *buffer);
extern void sd_zbc_remove(struct scsi_disk *sdkp);
extern void sd_zbc_print_zones(struct scsi_disk *sdkp);
extern int sd_zbc_setup_report_cmnd(struct scsi_cmnd *cmd);
extern int sd_zbc_setup_reset_cmnd(struct scsi_cmnd *cmd);
extern void sd_zbc_complete(struct scsi_cmnd *cmd, unsigned int good_bytes,
			    struct scsi_sense_hdr *sshdr);

#else /* CONFIG_BLK_DEV_ZONED */

static inline int sd_zbc_read_zones(struct scsi_disk *sdkp,
				    unsigned char *buf)
{
	return 0;
}

static inline void sd_zbc_remove(struct scsi_disk *sdkp) {}

static inline void sd_zbc_print_zones(struct scsi_disk *sdkp) {}

static inline int sd_zbc_setup_report_cmnd(struct scsi_cmnd *cmd)
{
	return BLKPREP_INVALID;
}

static inline int sd_zbc_setup_reset_cmnd(struct scsi_cmnd *cmd)
{
	return BLKPREP_INVALID;
}

static inline void sd_zbc_complete(struct scsi_cmnd *cmd,
				   unsigned int good_bytes,
				   struct scsi_sense_hdr *sshdr) {}

#endif /* CONFIG_BLK_DEV_ZONED */

#endif /* _SCSI_DISK_H */
