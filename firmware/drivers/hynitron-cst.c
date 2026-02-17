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

/* Hynitron CST series capacitive touch controller driver.
 *
 * Register layout for CST816S/CST816T (common Hynitron touch ICs):
 *   0x01: Gesture ID
 *   0x02: Number of touch points
 *   0x03: XH (event flag [7:6], X position [11:8])
 *   0x04: XL (X position [7:0])
 *   0x05: YH (touch ID [7:4], Y position [11:8])
 *   0x06: YL (Y position [7:0])
 *   0xA7: Chip ID
 *   0xA9: Firmware version
 *
 * The register layout is very similar to the FocalTech FT6x06 series. */

#include "hynitron-cst.h"
#include "kernel.h"
#include "i2c-async.h"
#include <string.h>

#define BYTES_PER_POINT 4

#ifdef HYNITRON_SWAP_AXES
# define POS_X pos_y
# define POS_Y pos_x
#else
# define POS_X pos_x
# define POS_Y pos_y
#endif

struct hynitron_driver {
    /* i2c bus data */
    int i2c_cookie;
    i2c_descriptor i2c_desc;

    /* callback for touch events */
    void(*event_cb)(struct hynitron_state *state);

    /* I2C buffer: reg addr + gesture + nr_points + pts */
    uint8_t raw_data[1 + 2 + BYTES_PER_POINT * HYNITRON_NUM_POINTS];
};

static struct hynitron_driver hyn_drv;
struct hynitron_state hynitron_state;

static inline void hynitron_convert_point(const uint8_t* raw,
                                          struct hynitron_point* pt)
{
    pt->event    = (raw[0] >> 6) & 0x3;
    pt->POS_X    = ((raw[0] & 0xf) << 8) | raw[1];
    pt->touch_id = (raw[2] >> 4) & 0xf;
    pt->POS_Y    = ((raw[2] & 0xf) << 8) | raw[3];
}

static void hynitron_i2c_callback(int status, i2c_descriptor* desc)
{
    (void)desc;
    if(status != I2C_STATUS_OK)
        return;

    hynitron_state.gesture = hyn_drv.raw_data[1];
    hynitron_state.nr_points = hyn_drv.raw_data[2] & 0xf;
    if(hynitron_state.nr_points > HYNITRON_NUM_POINTS)
        hynitron_state.nr_points = HYNITRON_NUM_POINTS;

    for(int i = 0; i < HYNITRON_NUM_POINTS; ++i) {
        hynitron_convert_point(&hyn_drv.raw_data[3 + i * BYTES_PER_POINT],
                               &hynitron_state.points[i]);
    }

    hyn_drv.event_cb(&hynitron_state);
}

static void hynitron_dummy_event_cb(struct hynitron_state* state)
{
    (void)state;
}

void hynitron_init(void)
{
    memset(&hyn_drv, 0, sizeof(hyn_drv));
    hyn_drv.event_cb = hynitron_dummy_event_cb;

    memset(&hynitron_state, 0, sizeof(struct hynitron_state));
    hynitron_state.gesture = -1;
    for(int i = 0; i < HYNITRON_NUM_POINTS; ++i)
        hynitron_state.points[i].event = HYNITRON_EVT_NONE;

    /* Reserve bus management cookie */
    hyn_drv.i2c_cookie = i2c_async_reserve_cookies(HYNITRON_BUS, 1);

    /* Prep an I2C descriptor to read touch data */
    hyn_drv.i2c_desc.slave_addr = HYNITRON_ADDR;
    hyn_drv.i2c_desc.bus_cond   = I2C_START | I2C_STOP;
    hyn_drv.i2c_desc.tran_mode  = I2C_READ;
    hyn_drv.i2c_desc.buffer[0]  = &hyn_drv.raw_data[0];
    hyn_drv.i2c_desc.count[0]   = 1;
    hyn_drv.i2c_desc.buffer[1]  = &hyn_drv.raw_data[1];
    hyn_drv.i2c_desc.count[1]   = sizeof(hyn_drv.raw_data) - 1;
    hyn_drv.i2c_desc.callback   = hynitron_i2c_callback;
    hyn_drv.i2c_desc.arg        = 0;
    hyn_drv.i2c_desc.next       = NULL;

    /* Start reading from register 0x01 (gesture) */
    hyn_drv.raw_data[0] = 0x01;
}

void hynitron_set_event_cb(void(*cb)(struct hynitron_state *state))
{
    hyn_drv.event_cb = cb ? cb : hynitron_dummy_event_cb;
}

void hynitron_enable(bool en)
{
    /* CST816S sleep mode: write 0x03 to register 0xA5
     * Wake: write 0x00 to register 0xA5 (or toggle reset pin) */
    i2c_reg_write1(HYNITRON_BUS, HYNITRON_ADDR, 0xa5, en ? 0 : 3);
}

void hynitron_irq_handler(void)
{
    i2c_async_queue(HYNITRON_BUS, TIMEOUT_NOBLOCK, I2C_Q_ONCE,
                    hyn_drv.i2c_cookie, &hyn_drv.i2c_desc);
}
