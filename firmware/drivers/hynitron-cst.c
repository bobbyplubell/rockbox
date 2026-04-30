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

/* Hynitron CST816T capacitive touch controller driver.
 *
 * Register layout (per CST816T SDK Register Spec v1.3, 2023-05-18):
 *   0x01: GestureID    — hardware-detected gesture (we disable these)
 *   0x02: FingerNum    — touch presence (0 = no finger, 1 = one finger)
 *   0x03: XposH        — bits [7:6] = Event Flag, bits [3:0] = X[11:8]
 *                          Event 00=press, 01=lift, 10=hold/move, 11=rsvd
 *   0x04: XposL        — X[7:0]
 *   0x05: YposH        — bits [3:0] = Y[11:8]
 *   0x06: YposL        — Y[7:0]
 *   0xA7: ChipID       0xA8: ProjID       0xA9: FwVersion
 *   0xE5: SleepMode    — write 0x03 to sleep
 *   0xEC: MotionMask   — bit 0 = EnDClick (only gesture register on T)
 *   0xFA: IrqCtl       — bit 6 = EnTouch, 5 = EnChange, 4 = EnMotion
 *   0xFE: DisAutoSleep — non-zero (<0xF0) disables auto-sleep
 *
 * The register layout is similar to the FocalTech FT6x06 series, but the
 * CST816S is NOT compatible (has no Event Flag in XposH).
 *
 * Touch presence is taken from FingerNum rather than the per-point Event
 * Flag — simpler, robust, and works the same on S/T variants. */

#include "hynitron-cst.h"
#include "i2c-x1000.h"
#include "kernel.h"
#include "i2c-async.h"
#include <string.h>

#define BYTES_PER_POINT 4

/* CST816S register addresses */
#define HYN_REG_GESTURE     0x01
#define HYN_REG_MOTIONMASK  0xEC
#define HYN_REG_IRQCTL      0xFA
#define HYN_REG_DISAUTOSLP  0xFE

/* IrqCtl bits */
#define HYN_IRQ_ENTOUCH     0x40  /* IRQ on touch detected */
#define HYN_IRQ_ENCHANGE    0x20  /* IRQ on coordinate change */
#define HYN_IRQ_ENMOTION    0x10  /* IRQ on hardware-detected gesture */

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
    /* CST816T: XposH bits [7:6] = Event Flag, [3:0] = X[11:8].
     *          YposH bits [3:0] = Y[11:8]. */
    pt->event = (raw[0] >> 6) & 0x3;
    pt->POS_X = ((raw[0] & 0xf) << 8) | raw[1];
    pt->POS_Y = ((raw[2] & 0xf) << 8) | raw[3];
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
    hyn_drv.raw_data[0] = HYN_REG_GESTURE;

    hynitron_configure_regs();
}

void hynitron_configure_regs(void)
{
    /* IrqCtl: fire IRQ on touch detected AND on every coordinate change.
     * EnChange is what enables continuous motion tracking — without it the
     * IC only interrupts on touch-down/lift, freezing coordinates mid-drag. */
    i2c_reg_write1(HYNITRON_BUS, HYNITRON_ADDR, HYN_REG_IRQCTL,
                   HYN_IRQ_ENTOUCH | HYN_IRQ_ENCHANGE);

    /* MotionMask=0: disable hardware gesture recognition (LR-scroll,
     * UD-scroll, double-click). Rockbox's software gesture engine handles
     * these consistently; running both produces conflicting events. */
    i2c_reg_write1(HYNITRON_BUS, HYNITRON_ADDR, HYN_REG_MOTIONMASK, 0);

    /* Disable auto-sleep so the IC keeps reporting during long touches
     * (default sleeps after 2s of no touch — a held finger can hit this). */
    i2c_reg_write1(HYNITRON_BUS, HYNITRON_ADDR, HYN_REG_DISAUTOSLP, 1);
}

void hynitron_set_event_cb(void(*cb)(struct hynitron_state *state))
{
    hyn_drv.event_cb = cb ? cb : hynitron_dummy_event_cb;
}

void hynitron_enable(bool en)
{
    /* Use the IC's auto-sleep (DisAutoSleep @0xFE), NOT SleepMode @0xE5.
     * SleepMode=0x03 is deep sleep that can only be woken via the reset
     * pin, and the reset wipes IrqCtl/MotionMask/DisAutoSleep — and doing
     * the reset pulse + I2C reconfigure on every keylock toggle (this is
     * called from action.c:do_key_lock) wedged the I2C bus, freezing
     * touch and the wheel encoder together.
     *
     * Auto-sleep is a light sleep that preserves all registers and the
     * IC auto-wakes on the next touch. A single I2C write per transition,
     * matching the FT6x06 pattern this driver was modeled on. The
     * driver-layer `touch_enabled` flag in firmware/drivers/touchscreen.c
     * filters spurious wakes during keylock.
     *
     * Default DisAutoSleep at init is 1 (auto-sleep disabled) so a held
     * finger never trips the ~2s timer during normal use. On disable we
     * flip it to 0 to allow the IC to sleep itself. */
    i2c_reg_write1(HYNITRON_BUS, HYNITRON_ADDR, HYN_REG_DISAUTOSLP, en ? 1 : 0);
}

void hynitron_irq_handler(void)
{
    i2c_async_queue(HYNITRON_BUS, TIMEOUT_NOBLOCK, I2C_Q_ONCE,
                    hyn_drv.i2c_cookie, &hyn_drv.i2c_desc);
}
