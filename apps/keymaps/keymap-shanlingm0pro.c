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

/* Button Code Definitions for Shanling M0 Pro target
 *
 * Matches bootloader mapping:
 *   VOL_UP (wheel CW)   = up / scroll up
 *   VOL_DOWN (wheel CCW) = down / scroll down
 *   CENTER (touch tap)   = select / OK / play
 *   POWER               = back / cancel / lock
 */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* {Action Code,    Button code,    Prereq button code } */

static const struct button_mapping button_context_standard[] = {
    {ACTION_STD_PREV,           BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_STD_PREVREPEAT,     BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
    {ACTION_STD_NEXT,           BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_STD_NEXTREPEAT,     BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    {ACTION_STD_OK,             BUTTON_CENTER|BUTTON_REL,           BUTTON_CENTER},
    {ACTION_STD_CONTEXT,        BUTTON_CENTER|BUTTON_REPEAT,        BUTTON_CENTER},
    {ACTION_STD_CANCEL,         BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[] = {
    {ACTION_WPS_PLAY,           BUTTON_CENTER|BUTTON_REL,           BUTTON_CENTER},
    {ACTION_WPS_STOP,           BUTTON_CENTER|BUTTON_REPEAT,        BUTTON_NONE},
    {ACTION_WPS_VOLUP,          BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_WPS_VOLUP,          BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
    {ACTION_WPS_VOLDOWN,        BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_WPS_VOLDOWN,        BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    {ACTION_WPS_SKIPNEXT,       BUTTON_MIDRIGHT|BUTTON_REL,         BUTTON_MIDRIGHT},
    {ACTION_WPS_SKIPPREV,       BUTTON_MIDLEFT|BUTTON_REL,          BUTTON_MIDLEFT},
    {ACTION_WPS_SEEKFWD,        BUTTON_MIDRIGHT|BUTTON_REPEAT,      BUTTON_NONE},
    {ACTION_WPS_STOPSEEK,       BUTTON_MIDRIGHT|BUTTON_REL,         BUTTON_MIDRIGHT|BUTTON_REPEAT},
    {ACTION_WPS_SEEKBACK,       BUTTON_MIDLEFT|BUTTON_REPEAT,       BUTTON_NONE},
    {ACTION_WPS_STOPSEEK,       BUTTON_MIDLEFT|BUTTON_REL,          BUTTON_MIDLEFT|BUTTON_REPEAT},
    {ACTION_WPS_BROWSE,         BUTTON_TOPMIDDLE|BUTTON_REL,        BUTTON_TOPMIDDLE},
    {ACTION_STD_KEYLOCK,        BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[] = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_yesno[] = {
    {ACTION_YESNO_ACCEPT,       BUTTON_CENTER,                      BUTTON_NONE},
    {ACTION_STD_CANCEL,         BUTTON_POWER,                       BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_yesno */

static const struct button_mapping button_context_settings[] = {
    {ACTION_SETTINGS_INC,       BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_SETTINGS_INCREPEAT, BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
    {ACTION_SETTINGS_DEC,       BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_SETTINGS_DECREPEAT, BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    {ACTION_STD_OK,             BUTTON_CENTER|BUTTON_REL,           BUTTON_CENTER},
    {ACTION_STD_CANCEL,         BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    LAST_ITEM_IN_LIST
}; /* button_context_settings */

const struct button_mapping* target_get_context_mapping(int context)
{
    switch (context & ~CONTEXT_LOCKED)
    {
        default:
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_TREE:
        case CONTEXT_CUSTOM|CONTEXT_TREE:
        case CONTEXT_MAINMENU:
        case CONTEXT_BOOKMARKSCREEN:
        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings;
    }
}
