/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by Aidan MacDonald
 * Copyright (C) 2021 by Dana Conrad
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

#include "button.h"
#include "touchscreen.h"
#include "hynitron-cst.h"
#include "kernel.h"
#include "backlight.h"
#include "powermgmt.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "i2c-x1000.h"
#include <stdbool.h>

#ifndef BOOTLOADER
# include "lcd.h"
# include "font.h"
#endif

/* Volume wheel rotation */
static volatile int wheel_pos = 0;

void button_init_device(void)
{
    /* Setup interrupts for the volume wheel */
    gpio_set_function(GPIO_WHEEL1, GPIOF_IRQ_EDGE(0));
    gpio_set_function(GPIO_WHEEL2, GPIOF_IRQ_EDGE(0));
    gpio_flip_edge_irq(GPIO_WHEEL1);
    gpio_flip_edge_irq(GPIO_WHEEL2);
    gpio_enable_irq(GPIO_WHEEL1);
    gpio_enable_irq(GPIO_WHEEL2);

    /* Init Hynitron touchscreen driver */
    i2c_x1000_set_freq(HYNITRON_BUS, I2C_FREQ_400K);
    hynitron_init();

    /* Reset touch controller */
    gpio_set_level(GPIO_HYNITRON_RESET, 0);
    mdelay(5);
    gpio_set_level(GPIO_HYNITRON_RESET, 1);
    mdelay(50);

    /* Jack detect GPIOs — PB6 = single-ended, PB7 = balanced (unused) */
    gpio_set_function(GPIO_PB(6), GPIOF_INPUT);
    gpio_set_function(GPIO_PB(7), GPIOF_INPUT);
    gpio_set_pull(GPIO_PB(6), 1);
    gpio_set_pull(GPIO_PB(7), 1);

    /* Enable Hynitron interrupt */
    system_set_irq_handler(GPIO_TO_IRQ(GPIO_HYNITRON_INTERRUPT),
                           hynitron_irq_handler);
    gpio_set_function(GPIO_HYNITRON_INTERRUPT, GPIOF_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_HYNITRON_INTERRUPT);
}

int button_read_device(int* data)
{
    const struct hynitron_point* point;
    int r = 0;

    /* Read power button GPIO — active low */
    uint32_t b = REG_GPIO_PIN(GPIO_B);
    if((b & (1 << 31)) == 0) r |= BUTTON_POWER;

    /* Check the wheel.
     *
     * Pattern adapted from the Sansa AMS scrollwheel driver
     * (firmware/target/arm/as3525/scrollwheel-as3525.c): self-pace by only
     * posting when the button queue has fully drained, preserve any leftover
     * quadrature in wheel_pos for the next tick (no event loss), and tag
     * fast spins with BUTTON_REPEAT so the keymap can route them
     * differently. The previous implementation posted at most one event per
     * tick and zeroed wheel_pos, silently dropping accumulated motion when
     * the consumer (e.g. WPS volume changes that hit the codec) lagged
     * behind the encoder. Also: no BUTTON_REL — apps/action.c special-cases
     * scrollwheel buttons via HAVE_SCROLLWHEEL and treats them as
     * auto-completed without a release. */
    int whpos = wheel_pos;
    if(whpos >= 4 || whpos <= -4) {
        static long last_wheel_post = 0;

        /* Wait for the consumer to drain before posting again. wheel_pos is
         * left untouched so accumulated detents are preserved. */
        if(button_queue_empty()) {
            int wheel_btn = (whpos > 0) ? BUTTON_VOL_DOWN : BUTTON_VOL_UP;

            /* Tag rapid spins as REPEAT (matches Sansa AMS behavior). */
            if(TIME_BEFORE(current_tick, last_wheel_post + HZ/10))
                wheel_btn |= BUTTON_REPEAT;

            /* Consume one detent (4 quadrature units), keep the remainder. */
            wheel_pos = (whpos > 0) ? whpos - 4 : whpos + 4;
            last_wheel_post = current_tick;

            button_queue_post(wheel_btn, 0);

            /* Poke the backlight */
            backlight_on();
            reset_poweroff_timer();
        }
    }

    /* Pass raw touch coordinates to Rockbox's software gesture layer
     * (apps/gesture.c) which handles tap, drag, drag-scroll and kinetic
     * scrolling consistently. With IrqCtl EnChange set, the IC fires an
     * interrupt on every coordinate change, queuing an async I2C read so
     * coordinates are fresh by the time button_tick runs.
     *
     * Touch-held state is tracked locally and only cleared on an explicit
     * Event=LIFT. Both nr_points and the per-point event field can flicker
     * mid-drag (the IC briefly reports zero fingers while updating, or a
     * stale event between IRQs); without this latch, button.c sees a
     * release+press cycle and action.c starts a fresh TOUCHEVENT_PRESS,
     * resetting the gesture origin to the current finger position so the
     * 20-px drag threshold is never crossed. */
    if(data) {
        static bool touch_held = false;
        point = &hynitron_state.points[0];
        int evt = point->event;

        /* Gate on FingerNum, not the per-point event byte: at boot the
         * zeroed state has event==0 which collides with HYNITRON_EVT_PRESS,
         * latching a phantom touch at (0,0) before the IC has been read. */
        if(hynitron_state.nr_points == 0 || evt == HYNITRON_EVT_LIFT)
            touch_held = false;
        else
            touch_held = true;

        if(touch_held) {
            int tx = (LCD_WIDTH - 1) - point->pos_x;
            int ty = (LCD_HEIGHT - 1) - point->pos_y;
            r |= touchscreen_to_pixels(tx, ty, data);
        }
    }

    return r;
}

void touchscreen_enable_device(bool en)
{
    hynitron_enable(en);
}

bool headphones_inserted(void)
{
    /* PB6 active low = jack inserted. Requires ALDO4 enabled. */
    return !gpio_get_level(GPIO_PB(6));
}

static void handle_wheel_irq(void)
{
    /* Wheel quadrature decoding — same as Q1/Eros Q */
    static const int delta[16] = { 0, -1,  1,  0,
                                   1,  0,  0, -1,
                                  -1,  0,  0,  1,
                                   0,  1, -1,  0 };
    static uint32_t state = 0;
    state <<= 2;
    state |= (REG_GPIO_PIN(GPIO_D) >> 2) & 3;
    state &= 0xf;

    wheel_pos += delta[state];
}

void GPIOD02(void)
{
    handle_wheel_irq();
    gpio_flip_edge_irq(GPIO_WHEEL1);
}

void GPIOD03(void)
{
    handle_wheel_irq();
    gpio_flip_edge_irq(GPIO_WHEEL2);
}

#ifndef BOOTLOADER
static int getbtn(void)
{
    int btn;
    do {
        btn = button_get_w_tmo(1);
    } while(btn & (BUTTON_REL|BUTTON_REPEAT));
    return btn;
}

bool dbg_shanlingm0pro_touchscreen(void)
{
    const int pad_w = LCD_WIDTH;
    const int pad_h = LCD_HEIGHT;
    const int box_h = pad_h - SYSFONT_HEIGHT*5;
    const int box_w = pad_w * box_h / pad_h;
    const int box_x = (LCD_WIDTH - box_w) / 2;
    const int box_y = SYSFONT_HEIGHT * 9 / 2;

    bool draw_border = true;

    do {
        int line = 0;
        lcd_clear_display();
        lcd_putsf(0, line++, "nr_points: %d  gesture: %d",
                  hynitron_state.nr_points, hynitron_state.gesture);

        if(draw_border)
            lcd_drawrect(box_x, box_y, box_w, box_h);

        for(int i = 0; i < hynitron_state.nr_points; ++i) {
            const struct hynitron_point* point = &hynitron_state.points[i];
            int cx = (LCD_WIDTH - 1) - point->pos_x;
            int cy = (LCD_HEIGHT - 1) - point->pos_y;
            lcd_putsf(0, line++, "pt%d evt:%d pos:%d,%d",
                      i, point->event, cx, cy);

            int tx = box_x + cx * box_w / pad_w;
            int ty = box_y + cy * box_h / pad_h;
            lcd_hline(tx-2, tx+2, ty);
            lcd_vline(tx, ty-2, ty+2);
        }

        lcd_update();
    } while(getbtn() != BUTTON_POWER);
    return false;
}
#endif
