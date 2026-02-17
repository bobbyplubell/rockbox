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

/* CST816S gesture IDs */
#define HYNITRON_GESTURE_NONE       0x00
#define HYNITRON_GESTURE_SWIPE_UP   0x01
#define HYNITRON_GESTURE_SWIPE_DOWN 0x02
#define HYNITRON_GESTURE_SWIPE_LEFT 0x03
#define HYNITRON_GESTURE_SWIPE_RIGHT 0x04
#define HYNITRON_GESTURE_SINGLE_TAP 0x05
#define HYNITRON_GESTURE_DOUBLE_TAP 0x0B
#define HYNITRON_GESTURE_LONG_PRESS 0x0C

enum hynitron_event {
    HYNITRON_EVT_NONE = -1,
    HYNITRON_EVT_PRESS = 0,
    HYNITRON_EVT_RELEASE = 1,
    HYNITRON_EVT_CONTACT = 2,
};

struct hynitron_point {
    int event;
    int touch_id;
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
