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

#ifndef __TERMINAL_APP_WRAPPER_H__
#define __TERMINAL_APP_WRAPPER_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HILDON == 0
#include <hildon-widgets/hildon-window.h>
#elif HILDON == 1
#include <hildon/hildon-window.h>
#endif

G_BEGIN_DECLS;

#define TERMINAL_TYPE_APP_WRAPPER            (terminal_app_wrapper_get_type ())
#define TERMINAL_APP_WRAPPER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_APP_WRAPPER, TerminalAppWrapper))
#define TERMINAL_APP_WRAPPER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_APP_WRAPPER, TerminalAppWrapperClass))
#define TERMINAL_IS_APP_WRAPPER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_APP_WRAPPER))
#define TERMINAL_IS_APP_WRAPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_APP_WRAPPER))
#define TERMINAL_APP_WRAPPER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_APP_WRAPPER, TerminalAppWrapperClass))

typedef struct _TerminalAppWrapperClass TerminalAppWrapperClass;
typedef struct _TerminalAppWrapper      TerminalAppWrapper;

struct _TerminalAppWrapperClass
{
  HildonWindowClass __parent__;

  /* signals */
  void (*new_window) (TerminalAppWrapper *app,
                      const gchar *working_directory);
};

struct _TerminalAppWrapper
{
  HildonWindow         __parent__;

  GtkActionGroup      *action_group;
  GtkUIManager        *ui_manager;

  gpointer         current;
  GSList              *apps;

  GSList *window_group;
  GtkWidget *windows_menu; /* Where window menuitems are*/

};

GType      terminal_app_wrapper_get_type (void) G_GNUC_CONST;
GtkWidget  *terminal_app_wrapper_new      (void);
gboolean   terminal_app_wrapper_launch (TerminalAppWrapper     *app,
    				const gchar     *command,
                                GError          **error);

G_END_DECLS;

#endif /* !__TERMINAL_APP_WRAPPER_H__ */
