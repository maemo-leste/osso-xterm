/**
  @file terminal-gconf.h

  @author Johan Hedberg <johan.hedberg@nokia.com>

  Copyright (C) 2004 Nokia Corporation

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#ifndef _TERMINAL_GCONF_H_
#define _TERMINAL_GCONF_H_

#include <string.h>

#define STREQ(a,b) (strcmp((a), (b)) == 0)

#define OSSO_XTERM_GCONF_PATH        "/apps/osso/xterm"

/* Integer */
#define OSSO_XTERM_GCONF_FONT_BASE_SIZE   OSSO_XTERM_GCONF_PATH "/font_size"
#define OSSO_XTERM_DEFAULT_FONT_BASE_SIZE 14

/* Integer */
#define OSSO_XTERM_GCONF_FONT_SIZE   OSSO_XTERM_GCONF_PATH "/font_size_delta"
#define OSSO_XTERM_DEFAULT_FONT_SIZE 0

#define OSSO_XTERM_GCONF_FONT_NAME   OSSO_XTERM_GCONF_PATH "/font_name"
#define OSSO_XTERM_DEFAULT_FONT_NAME "Bitstream Vera Sans Mono"

#define OSSO_XTERM_GCONF_FONT_COLOR   OSSO_XTERM_GCONF_PATH "/foreground"
#define OSSO_XTERM_DEFAULT_FONT_COLOR "#000000"

#define OSSO_XTERM_GCONF_BG_COLOR    OSSO_XTERM_GCONF_PATH "/background"
#define OSSO_XTERM_DEFAULT_BG_COLOR  "#FFFFFF"

/* Int */
#define OSSO_XTERM_GCONF_SCROLLBACK  OSSO_XTERM_GCONF_PATH "/scrollback"
#define OSSO_XTERM_DEFAULT_SCROLLBACK 200

/* Boolean */
#define OSSO_XTERM_GCONF_SCROLLBAR   OSSO_XTERM_GCONF_PATH "/scrollbar"
#define OSSO_XTERM_DEFAULT_SCROLLBAR TRUE

/* Boolean */
#define OSSO_XTERM_GCONF_TOOLBAR   OSSO_XTERM_GCONF_PATH "/toolbar"
#define OSSO_XTERM_DEFAULT_TOOLBAR TRUE

/* Boolean */
#define OSSO_XTERM_GCONF_TOOLBAR_FULLSCREEN   OSSO_XTERM_GCONF_PATH "/toolbar_fullscreen"
#define OSSO_XTERM_DEFAULT_TOOLBAR_FULLSCREEN TRUE

/* List of strings */
#define OSSO_XTERM_GCONF_KEYS   OSSO_XTERM_GCONF_PATH "/keys"

/* List of strings */
#define OSSO_XTERM_GCONF_KEY_LABELS   OSSO_XTERM_GCONF_PATH "/key_labels"

/* Boolean */
#define OSSO_XTERM_GCONF_REVERSE     OSSO_XTERM_GCONF_PATH "/reverse"
#define OSSO_XTERM_DEFAULT_REVERSE   FALSE

#define OSSO_XTERM_GCONF_ENCODING    OSSO_XTERM_GCONF_PATH "/encoding"
#define OSSO_XTERM_DEFAULT_ENCODING  "UTF-8"

#endif /* _TERMINAL_GCONF_H_ */
