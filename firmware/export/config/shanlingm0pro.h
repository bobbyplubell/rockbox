/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 by Alexander Polakov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* RoLo-related defines */
#define MODEL_NAME      "Shanling M0 Pro"
#define MODEL_NUMBER    125
#define BOOTFILE_EXT    "m0pro"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR         "/.rockbox"

/* CPU defines */
#define CONFIG_CPU          X1000
#define X1000_EXCLK_FREQ    24000000
#define CPU_FREQ            1008000000

#ifndef SIMULATOR
#define TIMER_FREQ          X1000_EXCLK_FREQ
#endif

/* Kernel defines */
#define INCLUDE_TIMEOUT_API
#define HAVE_SEMAPHORE_OBJECTS

/* Drivers */
#define HAVE_I2C_ASYNC

/* Buffer for plugins and codecs. */
#define PLUGIN_BUFFER_SIZE  0x200000 /* 2 MiB */
#define CODEC_SIZE          0x100000 /* 1 MiB */

/* LCD defines — 240x240 GC9A01 */
#define CONFIG_LCD          LCD_SHANLING_M0PRO
#define LCD_WIDTH           240
#define LCD_HEIGHT          240
#define LCD_DEPTH           16
#define LCD_PIXELFORMAT     RGB565
#define LCD_DPI             200
#define HAVE_LCD_COLOR
#define HAVE_LCD_BITMAP
#define HAVE_LCD_ENABLE
#define LCD_X1000_DMA_WAIT_FOR_FRAME

/* Backlight defines */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      100
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  70
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Headphone jack detection — PB6 active low, powered by ALDO4 */
#define HAVE_HEADPHONE_DETECTION

/* Codec / audio hardware defines */
#define HW_SAMPR_CAPS   SAMPR_CAP_ALL_192
#define HAVE_ES9218
#define HAVE_SW_TONE_CONTROLS

/* Button defines — M0 Pro has power button, rotary wheel, touchscreen */
#define CONFIG_KEYPAD   SHANLING_M0PRO_PAD
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA
#define HAVE_HYNITRON
#define HYNITRON_NUM_POINTS 1

/* Storage defines */
#define CONFIG_STORAGE  STORAGE_SD
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define HAVE_MULTIVOLUME
#define STORAGE_WANTS_ALIGN
#define STORAGE_NEEDS_BOUNCE_BUFFER

/* RTC settings */
#define CONFIG_RTC      RTC_X1000

/* Power management — AXP2101 */
#define CONFIG_BATTERY_MEASURE (VOLTAGE_MEASURE|PERCENTAGE_MEASURE)
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

#ifndef SIMULATOR
#define HAVE_AXP_PMU 192
#define HAVE_AXP2101_ADDON
#define HAVE_POWEROFF_WHILE_CHARGING
#endif

/* Only one battery type — M0 Pro has 650mAh per spec */
#define BATTERY_CAPACITY_DEFAULT 650
#define BATTERY_CAPACITY_MIN     650
#define BATTERY_CAPACITY_MAX     650
#define BATTERY_CAPACITY_INC     0

/* Multiboot */
#define HAVE_BOOTDATA
#define BOOT_REDIR "rockbox_main.shanling_m0pro"

/* USB support */
#ifndef SIMULATOR
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_TURNAROUND 5
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x0525
#define USB_PRODUCT_ID 0xa4a5
#define USB_DEVBSS_ATTR __attribute__((aligned(32)))
#define HAVE_USB_POWER
#define HAVE_USB_CHARGING_ENABLE
#define HAVE_USB_CHARGING_IN_THREAD
#define TARGET_USB_CHARGING_DEFAULT USB_CHARGING_FORCE
#define HAVE_BOOTLOADER_USB_MODE
#define USB_READ_BUFFER_SIZE    (128 * 1024)
#define USB_WRITE_BUFFER_SIZE   (128 * 1024)
#endif

#ifdef BOOTLOADER
/* Ignore on any key can cause surprising USB issues in the bootloader */
# define USBPOWER_BTN_IGNORE (~BUTTON_POWER)
#endif

/* Rockbox capabilities */
#define HAVE_FAT16SUPPORT
#define HAVE_ALBUMART
#define HAVE_BMP_SCALING
#define HAVE_JPEG
#define HAVE_TAGCACHE
#define HAVE_VOLUME_IN_LIST
#define HAVE_QUICKSCREEN
#define HAVE_HOTKEY
#define AB_REPEAT_ENABLE
#define HAVE_BOOTLOADER_SCREENDUMP
