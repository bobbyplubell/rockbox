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

#include "lcd.h"
#include "system.h"
#include "lcd-x1000.h"
#include "gpio-x1000.h"

/* LCD controller: GC9A01 (240x240 square TFT)
 * Stock firmware panel driver: "h0154_ic_dev"
 * Init sequence extracted from stock kernel (Linux 3.10.14-svn633).
 *
 * Bus: 16-bit parallel data, 8-bit commands (SLCD 8080 mode)
 * Pixel format: RGB565
 * MADCTL: 0x88 (MY=1, row address order inverted)
 * Row address offset: 80 (RASET 0x50-0x13F for 240 rows)
 */

static const uint32_t m0pro_lcd_cmd_enable[] = {
    /* GC9A01 inter-register enable */
    LCD_INSTR_CMD, 0xFE,
    LCD_INSTR_CMD, 0xEF,

    /* Memory Data Access Control */
    LCD_INSTR_CMD, 0x36,
    LCD_INSTR_DAT, 0x88,

    /* Interface Pixel Format — RGB565 */
    LCD_INSTR_CMD, 0x3A,
    LCD_INSTR_DAT, 0x05,

    /* Column Address Set: 0x00-0xEF (0-239) */
    LCD_INSTR_CMD, 0x2A,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0xEF,

    /* Row Address Set: 0x50-0x13F (80-319, panel memory offset) */
    LCD_INSTR_CMD, 0x2B,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x50,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x3F,

    /* GC9A01 internal registers */
    LCD_INSTR_CMD, 0x86,
    LCD_INSTR_DAT, 0x98,

    LCD_INSTR_CMD, 0x89,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x8B,
    LCD_INSTR_DAT, 0x80,

    LCD_INSTR_CMD, 0x8D,
    LCD_INSTR_DAT, 0x22,

    LCD_INSTR_CMD, 0x8E,
    LCD_INSTR_DAT, 0x0F,

    LCD_INSTR_CMD, 0xE8,
    LCD_INSTR_DAT, 0x12,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0xFF,
    LCD_INSTR_DAT, 0x62,

    LCD_INSTR_CMD, 0x99,
    LCD_INSTR_DAT, 0x3E,

    LCD_INSTR_CMD, 0x9D,
    LCD_INSTR_DAT, 0x4B,

    /* Power control */
    LCD_INSTR_CMD, 0xC3,
    LCD_INSTR_DAT, 0x27,

    LCD_INSTR_CMD, 0xC4,
    LCD_INSTR_DAT, 0x18,

    LCD_INSTR_CMD, 0xC9,
    LCD_INSTR_DAT, 0x1F,

    /* Positive gamma correction */
    LCD_INSTR_CMD, 0xF0,
    LCD_INSTR_DAT, 0x8F,
    LCD_INSTR_DAT, 0x1B,
    LCD_INSTR_DAT, 0x05,
    LCD_INSTR_DAT, 0x06,
    LCD_INSTR_DAT, 0x07,
    LCD_INSTR_DAT, 0x42,

    LCD_INSTR_CMD, 0xF2,
    LCD_INSTR_DAT, 0x5C,
    LCD_INSTR_DAT, 0x1F,
    LCD_INSTR_DAT, 0x12,
    LCD_INSTR_DAT, 0x10,
    LCD_INSTR_DAT, 0x07,
    LCD_INSTR_DAT, 0x43,

    /* Negative gamma correction */
    LCD_INSTR_CMD, 0xF1,
    LCD_INSTR_DAT, 0x59,
    LCD_INSTR_DAT, 0xCF,
    LCD_INSTR_DAT, 0xCF,
    LCD_INSTR_DAT, 0x35,
    LCD_INSTR_DAT, 0x37,
    LCD_INSTR_DAT, 0x8F,

    LCD_INSTR_CMD, 0xF3,
    LCD_INSTR_DAT, 0x58,
    LCD_INSTR_DAT, 0xCF,
    LCD_INSTR_DAT, 0xCF,
    LCD_INSTR_DAT, 0x35,
    LCD_INSTR_DAT, 0x37,
    LCD_INSTR_DAT, 0x8F,

    /* Tearing effect line on */
    LCD_INSTR_CMD, 0x35,
    LCD_INSTR_DAT, 0x00,

    /* Set tear scanline */
    LCD_INSTR_CMD, 0x44,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0xF0,

    /* Sleep out */
    LCD_INSTR_CMD, 0x11,
    LCD_INSTR_UDELAY, 120000,

    /* Display on */
    LCD_INSTR_CMD, 0x29,

    /* Memory write — ready for framebuffer data */
    LCD_INSTR_CMD, 0x2C,

    LCD_INSTR_END,
};

static const uint32_t m0pro_lcd_cmd_sleep[] = {
    LCD_INSTR_CMD, 0x28, /* Display off */
    LCD_INSTR_UDELAY, 20000,
    LCD_INSTR_CMD, 0x10, /* Sleep in */
    LCD_INSTR_UDELAY, 5000,
    LCD_INSTR_END,
};

static const uint32_t m0pro_lcd_cmd_wake[] = {
    LCD_INSTR_CMD, 0x11, /* Sleep out */
    LCD_INSTR_UDELAY, 120000,
    LCD_INSTR_CMD, 0x29, /* Display on */
    LCD_INSTR_UDELAY, 20000,
    LCD_INSTR_END,
};

/* DMA write command — GC9A01 memory write command is 0x2C */
static const uint8_t __attribute__((aligned(64)))
    m0pro_lcd_dma_wr_cmd[] = {0x2C, 0x2C, 0x2C, 0x2C};

const struct lcd_tgt_config lcd_tgt_config = {
    /* GC9A01: 16-bit parallel data bus, 8-bit command bus.
     * Extracted from stock kernel jzfb_platform_data:
     *   bus_width=16, cmd_width=8, 8080 mode, big endian. */
    .bus_width = 16,
    .cmd_width = 8,
    .use_6800_mode = 0,
    .use_serial = 0,
    .clk_polarity = 0,
    .dc_polarity = 0,
    .wr_polarity = 1,
    .te_enable = 0,
    .big_endian = 0,
    .dma_wr_cmd_buf = &m0pro_lcd_dma_wr_cmd,
    .dma_wr_cmd_size = sizeof(m0pro_lcd_dma_wr_cmd),
};

void lcd_tgt_enable(bool enable)
{
    if(enable) {
        /* Power on the panel */
        gpio_set_level(GPIO_LCD_PWR, 1);
        gpio_set_level(GPIO_LCD_RST, 1);
        gpio_set_level(GPIO_LCD_CE, 1);
        gpio_set_level(GPIO_LCD_RD, 1);
        mdelay(50);
        gpio_set_level(GPIO_LCD_RST, 0);
        mdelay(100);
        gpio_set_level(GPIO_LCD_RST, 1);
        mdelay(50);
        gpio_set_level(GPIO_LCD_CE, 0);

        /* Start the controller */
        lcd_set_clock(X1000_CLK_SCLK_A, 20000000);
        lcd_exec_commands(m0pro_lcd_cmd_enable);
    } else {
        /* Power down: put panel into sleep, then cut power GPIOs */
        lcd_exec_commands(m0pro_lcd_cmd_sleep);
        gpio_set_level(GPIO_LCD_CE, 1);
        gpio_set_level(GPIO_LCD_RST, 0);
        gpio_set_level(GPIO_LCD_PWR, 0);
    }
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(m0pro_lcd_cmd_sleep);
    else
        lcd_exec_commands(m0pro_lcd_cmd_wake);
}
