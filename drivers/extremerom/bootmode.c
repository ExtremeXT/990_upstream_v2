/*
 * Copyright (c) 2025, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/libfdt.h>
#include <linux/printk.h>
#include <linux/types.h>

#include <linux/extremerom/bootmode.h>

static enum extremerom_bootmode extremerom_current_bootmode = BOOTMODE_UNKNOWN;

enum extremerom_bootmode get_extremerom_bootmode(void)
{
	return extremerom_current_bootmode;
}

static int __init extremerom_bootmode_init(void)
{
	printk(KERN_INFO "ExtremeROM: checking for bootmode\n");

	if (!strlen(boot_command_line))
	{
		printk(KERN_ERR "ExtremeROM: boot_command_line is empty\n");
		return -1;
	}

	/*
	 * Ok, we have a boot command line.
	 * Let's check it if it's a charger boot (androidboot.mode=charger)
	 * or, see if bootmode=2 is set for recovery.
	 * If neither are, it's a normal boot.
	*/
	if (strstr(boot_command_line, "androidboot.mode=charger"))
	{
		printk(KERN_INFO "ExtremeROM: bootmode=charger\n");
		extremerom_current_bootmode = BOOTMODE_CHARGER;
		return 0;
	}

	if (strstr(boot_command_line, "bootmode=2"))
	{
		printk(KERN_INFO "ExtremeROM: bootmode=recovery\n");
		extremerom_current_bootmode = BOOTMODE_RECOVERY;
		return 0;
	}

	printk(KERN_INFO "ExtremeROM: bootmode=normal\n");
	extremerom_current_bootmode = BOOTMODE_NORMAL;

	return 0;
}

subsys_initcall(extremerom_bootmode_init);