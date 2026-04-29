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

#ifndef __HYNITRON_CST_H__
#define __HYNITRON_CST_H__

#include "config.h"
#include <stdbool.h>

/* Hynitron CST series capacitive touch controller driver.
 * Modeled after the FT6x06 driver. Supports CST816S, CST816T, CST226SE
 * and similar Hynitron touch ICs commonly found in small displays. */

/* CST816S hardware-detected gesture IDs (register 0x01).
 * We disable hardware gesture recognition (MotionMask=0) and use Rockbox's
 * software gesture engine instead, but the IDs are kept in case any future
 * code wants to act on them. */
#define HYNITRON_GESTURE_NONE        0x00
#define HYNITRON_GESTURE_SWIPE_UP    0x01
#define HYNITRON_GESTURE_SWIPE_DOWN  0x02
#define HYNITRON_GESTURE_SWIPE_LEFT  0x03
#define HYNITRON_GESTURE_SWIPE_RIGHT 0x04
#define HYNITRON_GESTURE_SINGLE_TAP  0x05
#define HYNITRON_GESTURE_DOUBLE_TAP  0x0B
#define HYNITRON_GESTURE_LONG_PRESS  0x0C

/* CST816T per-point Event Flag values (XposH bits [7:6]).
 * The CST816S does NOT have this field — bits are reserved there. */
#define HYNITRON_EVT_PRESS    0   /* finger just touched down */
#define HYNITRON_EVT_LIFT     1   /* finger lifted off */
#define HYNITRON_EVT_HOLD     2   /* finger held or moving */

struct hynitron_point {
    int event;
    int pos_x;
    int pos_y;
};

struct hynitron_state {
    int gesture;
    int nr_points;
    struct hynitron_point points[HYNITRON_NUM_POINTS];
};

extern struct hynitron_state hynitron_state;

void hynitron_init(void);
void hynitron_set_event_cb(void(*fn)(struct hynitron_state *state));
void hynitron_enable(bool en);
void hynitron_irq_handler(void);

#endif /* __HYNITRON_CST_H__ */
