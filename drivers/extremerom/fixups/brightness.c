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

#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/types.h>

#include <linux/extremerom/bootmode.h>

static bool bootanim_set_brightness = false;

int hwc_get_brightness(int brightness)
{
	// Only work around this if we are past boot animation.
	if (bootanim_set_brightness)
		return brightness * 100;
	else
		bootanim_set_brightness = true;

	return brightness;
}

int get_fixed_brightness(int brightness)
{
	/*
	 * We have really three ways to go from here.
	 * 1. We are in Framebuffer-based UIs (recovery, charger).
	 *    Skip the brightness hack.
	 *
	 * 2. We are in system, but we are in bootanimation.
	 *    Skip the brightness hack. (handled in hwc_get_brightness)
	 *
	 * 3. We are in system, and we are not in bootanimation.
	 *    Apply the brightness hack.
	*/

	if(likely(get_extremerom_bootmode() == BOOTMODE_NORMAL))
		return hwc_get_brightness(brightness);
	else
		return brightness;
}

EXPORT_SYMBOL_GPL(get_fixed_brightness);