/*-
 * Copyright (c) 2004 os-cillation e.K.
 * maemo specific changes: Copyright (c) 2005 Nokia Corporation
 *
 * Written by Benedikt Meurer <benny@xfce.org>.
 * maemo specific changes by Johan Hedberg <johan.hedberg@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TERMINAL_APP_H__
#define __TERMINAL_APP_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HILDON == 0
#include <hildon-widgets/hildon-window.h>
#elif HILDON == 1
#include <hildon/hildon-window.h>
#endif

#include "terminal-widget.h"

G_BEGIN_DECLS;

#define TERMINAL_TYPE_APP            (terminal_app_get_type ())
#define TERMINAL_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_APP, TerminalApp))
#define TERMINAL_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_APP, TerminalAppClass))
#define TERMINAL_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_APP))
#define TERMINAL_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_APP))
#define TERMINAL_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_APP, TerminalAppClass))

typedef struct _TerminalAppClass TerminalAppClass;
typedef struct _TerminalApp      TerminalApp;

struct _TerminalAppClass
{
  HildonWindowClass __parent__;

  /* signals */
  void (*new_window) (TerminalApp *app,
                      const gchar *working_directory);
};

GType      terminal_app_get_type (void) G_GNUC_CONST;

GtkWidget *terminal_app_new      (void);

GtkWidget *terminal_app_add      (TerminalApp    *app,
                                  TerminalWidget *widget);

void       terminal_app_remove   (TerminalApp *app,
                                  TerminalWidget *widget);

gboolean   terminal_app_launch (TerminalApp     *app,
    				const gchar     *command,
                                GError          **error);

void terminal_app_new_window (TerminalApp  *app);

void terminal_app_set_state      (TerminalApp    *app);

G_END_DECLS;

#endif /* !__TERMINAL_APP_H__ */
