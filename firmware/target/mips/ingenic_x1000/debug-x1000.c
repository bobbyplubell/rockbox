/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef BOOTLOADER
#include "system.h"
#include "kernel.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "action.h"
#include "list.h"

#include "clk-x1000.h"
#include "gpio-x1000.h"

#ifdef SHANLING_M0PRO
#include "i2c-x1000.h"
#include "i2c-target.h"
#include "axp-2101.h"
#endif

static bool dbg_clocks(void)
{
    do {
        lcd_clear_display();
        int line = 0;
        for(int i = 0; i < X1000_CLK_COUNT; ++i) {
            uint32_t hz = clk_get(i);
            uint32_t khz = hz / 1000;
            uint32_t mhz = khz / 1000;
            lcd_putsf(2, line++, "%8s  %4u,%03u,%03u Hz", clk_get_name(i),
                      mhz, (khz - mhz*1000), (hz - khz*1000));
        }

        lcd_update();
    } while(get_action(CONTEXT_STD, HZ) != ACTION_STD_CANCEL);

    return false;
}

static void dbg_gpios_show_state(void)
{
    const char portname[] = "ABCD";
    for(int i = 0; i < 4; ++i)
        lcd_putsf(0, i, "GPIO %c: %08x", portname[i], REG_GPIO_PIN(i));
}

static void dbg_gpios_show_config(void)
{
    const char portname[] = "ABCD";
    int line = 0;
    for(int i = 0; i < 4; ++i) {
        uint32_t intr = REG_GPIO_INT(i);
        uint32_t mask = REG_GPIO_MSK(i);
        uint32_t pat0 = REG_GPIO_PAT0(i);
        uint32_t pat1 = REG_GPIO_PAT1(i);
        lcd_putsf(0, line++, "GPIO %c", portname[i]);
        lcd_putsf(2, line++, " int %08lx", intr);
        lcd_putsf(2, line++, " msk %08lx", mask);
        lcd_putsf(2, line++, "pat0 %08lx", pat0);
        lcd_putsf(2, line++, "pat1 %08lx", pat1);
        line++;
    }
}

static bool dbg_gpios(void)
{
    enum { STATE, CONFIG, NUM_SCREENS };
    const int timeouts[NUM_SCREENS] = { 1, HZ };
    int screen = STATE;

    while(1) {
        lcd_clear_display();
        switch(screen) {
        case CONFIG:
            dbg_gpios_show_config();
            break;
        case STATE:
            dbg_gpios_show_state();
            break;
        }

        lcd_update();

        switch(get_action(CONTEXT_STD, timeouts[screen])) {
        case ACTION_STD_CANCEL:
            return false;
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            screen -= 1;
            if(screen < 0)
                screen = NUM_SCREENS - 1;
            break;
        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            screen += 1;
            if(screen >= NUM_SCREENS)
                screen = 0;
            break;
        default:
            break;
        }
    }

    return false;
}

extern volatile unsigned aic_tx_underruns;
#ifdef HAVE_RECORDING
extern volatile unsigned aic_rx_overruns;
#endif
#ifdef HAVE_EROS_QN_CODEC
extern int es9018k2m_present_flag;
#endif

static bool dbg_audio(void)
{
    do {
        lcd_clear_display();
        lcd_putsf(0, 0, "TX underruns: %u", aic_tx_underruns);
#ifdef HAVE_RECORDING
        lcd_putsf(0, 1, "RX overruns:  %u", aic_rx_overruns);
#endif
#ifdef HAVE_EROS_QN_CODEC
        if (es9018k2m_present_flag)
        {
            lcd_putsf(0, 2, "(%d) ES9018K2M HWVOL", es9018k2m_present_flag);
        }
        else
        {
            lcd_putsf(0, 2, "(%d) SWVOL", es9018k2m_present_flag);
        }
#endif
        lcd_update();
    } while(get_action(CONTEXT_STD, HZ) != ACTION_STD_CANCEL);

    return false;
}

#ifdef SHANLING_M0PRO
/* Raw register dump for AXP2101 — used to verify what stock SPL programmed
 * before Rockbox boots (CV target, charge current, iterm, gauge config)
 * since we never write these explicitly. */
static bool dbg_axp2101_regs(void)
{
    enum { PAGE_PARSED_CHG, PAGE_PARSED_BAT, PAGE_DUMP_LO, PAGE_DUMP_HI, NUM_PAGES };
    int page = PAGE_PARSED_CHG;

    while(1) {
        lcd_clear_display();

        if(page == PAGE_PARSED_CHG || page == PAGE_PARSED_BAT) {
            int line = 0;
            int r;
            int vbat = axp2101_adc_read(AXP2101_ADC_VBAT_VOLTAGE);
            int vbus = axp2101_adc_read(AXP2101_ADC_VBUS_VOLTAGE);
            int vsys = axp2101_adc_read(AXP2101_ADC_VSYS_VOLTAGE);
            int soc  = axp2101_egauge_read();

            lcd_putsf(0, line++, "AXP2101 %s",
                      page == PAGE_PARSED_CHG ? "charge" : "battery");
            lcd_putsf(0, line++, "Vbat=%d Vbus=%d", vbat, vbus);
            lcd_putsf(0, line++, "Vsys=%d SoC=%d%%", vsys, soc);
            line++;

            if(page == PAGE_PARSED_CHG) {
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_PMU_STATUS1);
                lcd_putsf(0, line++, "00 STAT1: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_PMU_STATUS2);
                lcd_putsf(0, line++, "01 STAT2: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_INVOLTLIMITCTRL);
                lcd_putsf(0, line++, "15 VINDPM: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_INCURRLIMITCTRL);
                lcd_putsf(0, line++, "16 IINLIM: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_PERIPHERALCTRL);
                lcd_putsf(0, line++, "18 PERIPH: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_IPRECHG_SETTING);
                lcd_putsf(0, line++, "61 IPRECHG: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_ICC_SETTING);
                lcd_putsf(0, line++, "62 ICC: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_ITERM_SETTING);
                lcd_putsf(0, line++, "63 ITERM: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_CV_SETTING);
                lcd_putsf(0, line++, "64 CV: %02x", r & 0xff);
            } else {
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_THERMREGTHRESH);
                lcd_putsf(0, line++, "65 THERM: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_CHARGETIMEOUT);
                lcd_putsf(0, line++, "67 TIMEOUT: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_BATTDETECTCTRL);
                lcd_putsf(0, line++, "68 BATDET: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_BATT_PARAMETER);
                lcd_putsf(0, line++, "a1 BATPARM: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_FUEL_GAUGE_CTRL);
                lcd_putsf(0, line++, "a2 GAUGE: %02x", r & 0xff);
                r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_BATT_PERCENTAGE);
                lcd_putsf(0, line++, "a4 SOC: %02x", r & 0xff);
            }
        } else {
            int base = (page == PAGE_DUMP_LO) ? 0x00 : 0x80;
            lcd_putsf(0, 0, "Regs %02x-%02x", base, base + 0x7f);
            for(int row = 0; row < 16; row++) {
                int off = base + row * 8;
                int v[8];
                for(int c = 0; c < 8; c++)
                    v[c] = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, off + c) & 0xff;
                lcd_putsf(0, row + 1, "%02x:%02x %02x %02x %02x %02x %02x %02x %02x",
                          off, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
            }
        }

        lcd_update();

        switch(get_action(CONTEXT_STD, HZ)) {
        case ACTION_STD_CANCEL:
            return false;
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            page = (page + NUM_PAGES - 1) % NUM_PAGES;
            break;
        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            page = (page + 1) % NUM_PAGES;
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef X1000_CPUIDLE_STATS
static bool dbg_cpuidle(void)
{
    do {
        lcd_clear_display();
        lcd_putsf(0, 0, "CPU idle time: %d.%01d%%",
                  __cpu_idle_cur/10, __cpu_idle_cur%10);
        lcd_putsf(0, 1, "CPU frequency: %d.%03d MHz",
                  FREQ/1000000, (FREQ%1000000)/1000);
        lcd_update();
    } while(get_action(CONTEXT_STD, HZ) != ACTION_STD_CANCEL);

    return false;
}
#endif

#ifdef FIIO_M3K
extern bool dbg_fiiom3k_touchpad(void);
#endif
#ifdef SHANLING_Q1
extern bool dbg_shanlingq1_touchscreen(void);
#endif
#ifdef SHANLING_M0PRO
extern bool dbg_shanlingm0pro_touchscreen(void);
#endif
#ifdef HAVE_AXP_PMU
extern bool axp_debug_menu(void);
#endif
#ifdef HAVE_CW2015
extern bool cw2015_debug_menu(void);
#endif

/* Menu definition */
static const struct {
    const char* name;
    bool(*function)(void);
} menuitems[] = {
    {"Clocks", &dbg_clocks},
    {"GPIOs", &dbg_gpios},
#ifdef X1000_CPUIDLE_STATS
    {"CPU idle", &dbg_cpuidle},
#endif
    {"Audio", &dbg_audio},
#ifdef FIIO_M3K
    {"Touchpad", &dbg_fiiom3k_touchpad},
#endif
#ifdef SHANLING_Q1
    {"Touchscreen", &dbg_shanlingq1_touchscreen},
#endif
#ifdef SHANLING_M0PRO
    {"Touchscreen", &dbg_shanlingm0pro_touchscreen},
    {"AXP2101 regs", &dbg_axp2101_regs},
#endif
#ifdef HAVE_AXP_PMU
    {"Power stats", &axp_debug_menu},
#endif
#ifdef HAVE_CW2015
    {"CW2015 debug", &cw2015_debug_menu},
#endif
};

static int hw_info_menu_action_cb(int btn, struct gui_synclist* lists)
{
    if(btn == ACTION_STD_OK) {
        int sel = gui_synclist_get_sel_pos(lists);
        FOR_NB_SCREENS(i)
            viewportmanager_theme_enable(i, false, NULL);

        lcd_setfont(FONT_SYSFIXED);
        lcd_set_foreground(LCD_WHITE);
        lcd_set_background(LCD_BLACK);

        if(menuitems[sel].function())
            btn = SYS_USB_CONNECTED;
        else
            btn = ACTION_REDRAW;

        lcd_setfont(FONT_UI);

        FOR_NB_SCREENS(i)
            viewportmanager_theme_undo(i, false);
    }

    return btn;
}

static const char* hw_info_menu_get_name(int item, void* data,
                                         char* buffer, size_t buffer_len)
{
    (void)buffer;
    (void)buffer_len;
    (void)data;
    return menuitems[item].name;
}

bool dbg_hw_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, MODEL_NAME " debug menu",
                         ARRAYLEN(menuitems), NULL);
    info.action_callback = hw_info_menu_action_cb;
    info.get_name = hw_info_menu_get_name;
    return simplelist_show_list(&info);
}

bool dbg_ports(void)
{
    return false;
}
#endif
