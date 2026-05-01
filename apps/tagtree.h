/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
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
#ifdef HAVE_TAGCACHE
#ifndef _TAGTREE_H
#define _TAGTREE_H

#include "config.h"
#include "tagcache.h"
#include "tree.h"

#define TAGNAVI_VERSION    "#! rockbox/tagbrowser/2.0"
#define TAGMENU_MAX_ITEMS  64
#define TAGMENU_MAX_MENUS  32
#define TAGMENU_MAX_FMTS   32

int tagtree_export(void);
int tagtree_import(void);
void tagtree_init(void) INIT_ATTR;
int tagtree_enter(struct tree_context* c, bool is_visible);
void tagtree_exit(struct tree_context* c, bool is_visible);
int tagtree_load(struct tree_context* c);

/* Jump directly into the "Same as currently played track" tagnavi submenu
 * for the field of the currently-playing track, as if the user navigated
 * Database -> "Same..." -> field manually. Caller should follow up with
 * goto_root_menu(GO_TO_DBBROWSER) so the database tree picks up the primed
 * state. Returns false if no track is playing, or if the user-customized
 * tagnavi.config has no "same" menu / matching field entry. */
enum tagtree_goto_field {
    TAGTREE_GOTO_ARTIST,
    TAGTREE_GOTO_ALBUM,
    TAGTREE_GOTO_COMPOSER,
    TAGTREE_GOTO_TITLE,
};
bool tagtree_subentries_do(struct tree_context *c, enum tagtree_goto_field field);

/* Returns true (peek; does not clear) if a tagtree_subentries_do() call has
 * armed a jump and is awaiting the screen-stack to land in the DB browser.
 * The wrapper around the launching screen (e.g. browse_id3_wrapper) checks
 * this on exit and returns GO_TO_DBBROWSER instead of GO_TO_PREVIOUS. */
bool tagtree_consume_pending_db_jump(void);

/* Apply the pending jump to a tree_context (typically called from dirbrowse
 * after rockbox_browse()'s reset of tc->dirlevel/selected_item). No-op if
 * no jump is pending. Always clears the pending flag. */
void tagtree_apply_pending_jump(struct tree_context *c);

char* tagtree_get_entry_name(struct tree_context *c, int id,
                                    char* buf, size_t bufsize);
bool tagtree_current_playlist_insert(int position, bool queue);
int tagtree_add_to_playlist(const char* playlist, bool new_playlist);
char *tagtree_get_title(struct tree_context* c);
int tagtree_get_attr(struct tree_context* c);
int tagtree_get_icon(struct tree_context* c);
int tagtree_get_filename(struct tree_context* c, char *buf, int buflen);
int tagtree_get_custom_action(struct tree_context* c);
bool tagtree_get_subentry_filename(char *buf, size_t bufsize);
bool tagtree_subentries_do_action(bool (*action_cb)(const char *file_name));

#endif
#endif
