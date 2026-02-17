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

/* ES9219C DAC driver — register-compatible with ES9218P.
 * Adapted from audiohw-shanlingq1.c with M0 Pro GPIO pin names.
 *
 * Audio routing:
 *   Single-ended: I2S (AIC) → ES9219C DAC1 (0x48) → DAC1_HP_EN (PB10) → 3.5mm jack
 *   Balanced:     I2S (AIC) → ES9219C DAC1 (0x48) → DAC1_HP_EN (PB10) → one channel
 *                 I2S (AIC) → ES9219C DAC2 (0x49) → DAC2_HP_EN (PB11) → other channel
 *
 * Both DACs share the power GPIO (PB13) and reset (PD5).
 * In single-ended mode, only DAC1 is active.
 * In balanced mode, both DACs are active (channel assignment unverified),
 * accessible with a 2.5mm balanced adapter cable.
 * Balanced output is not yet implemented; DAC2 is put into low-power state. */

#include "audiohw.h"
#include "system.h"
#include "pcm_sampr.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"
#include "x1000/aic.h"
#include "x1000/cpm.h"

#define CODEC_MASTER_MODE 0

static int cur_fsel = HW_FREQ_48;
static int cur_vol_l = 0, cur_vol_r = 0;
static int cur_filter = 0;
static enum es9218_amp_mode cur_amp_mode = ES9218_AMP_MODE_1VRMS;

static void codec_start(void)
{
    es9218_open();
    es9218_mute(true);
    es9218_set_iface_role(CODEC_MASTER_MODE ? ES9218_IFACE_ROLE_MASTER
                                            : ES9218_IFACE_ROLE_SLAVE);
    es9218_set_iface_format(ES9218_IFACE_FORMAT_I2S, ES9218_IFACE_BITS_32);
    es9218_set_dpll_bandwidth(10);
    es9218_set_thd_compensation(true);
    es9218_set_thd_coeffs(0, 0);
    audiohw_set_filter_roll_off(cur_filter);
    audiohw_set_frequency(cur_fsel);
    audiohw_set_volume(cur_vol_l, cur_vol_r);
    es9218_set_amp_mode(cur_amp_mode);
}

static void codec_stop(void)
{
    es9218_mute(true);
    es9218_close();
    mdelay(4);
}

void audiohw_init(void)
{
    /* Configure AIC */
    aic_set_external_codec(true);
    aic_set_i2s_mode(CODEC_MASTER_MODE ? AIC_I2S_SLAVE_MODE
                                       : AIC_I2S_MASTER_MODE);
    aic_enable_i2s_bit_clock(true);

    /* HP pins (PB10/PB11) left in default state here — not touched until
     * audiohw_postinit to avoid gpio_set_function glitch during DAC power-on */

    /* Open DAC driver — only DAC1 (0x48) is used for single-ended output. */
    i2c_x1000_set_freq(ES9218_BUS, I2C_FREQ_400K);
    codec_start();

    /* Set DAC2 (0x49) amp mode to "Core on" (reg 0x20 = 0x00) to prevent
     * startup pop. This keeps the DAC core active but the headphone amp
     * not driving. DAC2 is unused in single-ended mode. */
    i2c_reg_write1(ES9218_BUS, ES9218_ADDR2, ES9218_REG_AMP_CONFIG, 0x00);
}

void audiohw_postinit(void)
{
    /* DAC is initialized and muted. Wait for output to fully settle. */
    mdelay(500);

    /* Configure and enable HP in one step — go directly to output-high.
     * Avoids intermediate output-low state. */
    gpio_set_function(GPIO_PB(10), GPIOF_OUTPUT(1));  /* DAC1_HP_EN on */
    mdelay(200);

    /* Now unmute — signal ramps in smoothly via DAC soft-start */
    es9218_mute(false);
}

void audiohw_close(void)
{
    /* Mute HP output before stopping DAC to prevent shutdown pop.
     * Pins are outputs after postinit, so gpio_set_level works. */
    gpio_set_level(GPIO_PB(10), 0);  /* DAC1_HP_EN off */
    mdelay(400);
    es9218_mute(true);
    es9218_close();
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];

    enum es9218_clock_gear clkgear;
    if(sampr <= 48000)
        clkgear = ES9218_CLK_GEAR_4;
    else if(sampr <= 96000)
        clkgear = ES9218_CLK_GEAR_2;
    else
        clkgear = ES9218_CLK_GEAR_1;

    aic_enable_i2s_bit_clock(false);
    es9218_set_clock_gear(clkgear);

    if(CODEC_MASTER_MODE)
        es9218_set_nco_frequency(sampr);
    else
        aic_set_i2s_clock(X1000_CLK_SCLK_A, sampr, 64);

    aic_enable_i2s_bit_clock(true);
    cur_fsel = fsel;
}

static int round_step_up(int x, int step)
{
    int rem = x % step;
    if(rem > 0)
        rem -= step;
    return x - rem;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    cur_vol_l = vol_l;
    cur_vol_r = vol_r;

    int amp = round_step_up(MAX(vol_l, vol_r), ES9218_AMP_VOLUME_STEP);
    amp = MIN(amp, ES9218_AMP_VOLUME_MAX);
    amp = MAX(amp, ES9218_AMP_VOLUME_MIN);

    vol_l -= amp;
    vol_l = MIN(vol_l, ES9218_DIG_VOLUME_MAX);
    vol_l = MAX(vol_l, ES9218_DIG_VOLUME_MIN);

    vol_r -= amp;
    vol_r = MIN(vol_r, ES9218_DIG_VOLUME_MAX);
    vol_r = MAX(vol_r, ES9218_DIG_VOLUME_MIN);

    es9218_set_amp_volume(amp);
    es9218_set_dig_volume(vol_l, vol_r);
}

void audiohw_set_filter_roll_off(int value)
{
    cur_filter = value;
    es9218_set_filter(value);
}

void audiohw_set_power_mode(int mode)
{
    enum es9218_amp_mode new_amp_mode;
    if(mode == SOUND_HIGH_POWER)
        new_amp_mode = ES9218_AMP_MODE_2VRMS;
    else
        new_amp_mode = ES9218_AMP_MODE_1VRMS;

    if(new_amp_mode != cur_amp_mode) {
        codec_stop();
        cur_amp_mode = new_amp_mode;
        codec_start();
        es9218_mute(false);
    }
}

void es9218_set_power_pin(int level)
{
    gpio_set_level(GPIO_ES9218_POWER, level ? 1 : 0);
}

void es9218_set_reset_pin(int level)
{
    gpio_set_level(GPIO_ES9218_RESET, level ? 1 : 0);
}

uint32_t es9218_get_mclk(void)
{
    /* TODO: Measure actual MCLK on M0 Pro by reading DPLL registers
     * while playing 44.1 KHz audio. Using Q1's value as placeholder. */
    return 38400000;
}
