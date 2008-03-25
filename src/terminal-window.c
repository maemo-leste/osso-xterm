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
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 * The geometry handling code was taken from gnome-terminal. The geometry hacks
 * where initially written by Owen Taylor.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libosso.h>

#ifdef HAVE_OSSO_BROWSER
#include <osso-browser-interface.h>
#else
#include <tablet-browser-interface.h>
#endif

#include <libintl.h>
#include <locale.h>

#define GETTEXT_PACKAGE "osso-browser-ui"
#define _(String) gettext(String)
#define N_(String) String

#if HILDON == 0
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#elif HILDON == 1
#include <hildon/hildon-window.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#endif
#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>

#include "terminal-gconf.h"
#include "terminal-settings.h"
#include "terminal-tab-header.h"
#include "terminal-window.h"
#include "terminal-encoding.h"
#include "shortcuts.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

/* signals */
enum
{
  SIGNAL_NEW_WINDOW = 1,
  SIGNAL_WINDOW_STATE_CHANGED,
  LAST_SIGNAL
};

static guint terminal_window_signals[LAST_SIGNAL] = { 0 };

static void            terminal_window_dispose                 (GObject         *object);
static void            terminal_window_finalize                (GObject         *object);
static void            terminal_window_update_actions          (TerminalWindow     *window);
static void            terminal_window_context_menu            (TerminalWidget  *widget,
                                                             GdkEvent        *event,
                                                             TerminalWindow     *window);
static void            terminal_window_notify_title            (TerminalWidget  *terminal,
                                                             GParamSpec      *pspec,
                                                               TerminalWindow     *window);
static void            terminal_window_open_url                (GtkAction       *action,
		                                             TerminalWindow     *window);
static void            terminal_window_action_new_window          (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_close_tab        (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_edit             (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_copy             (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_paste            (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_edit_shortcuts   (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reverse          (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_fullscreen       (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_scrollbar       (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_toolbar        (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_font_size        (GtkRadioAction  *action,
                                                             GtkRadioAction  *current,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reset            (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reset_and_clear  (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_ctrl             (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_encoding         (GtkAction       *action,
								TerminalWindow  *window);

static void            terminal_window_action_settings         (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_quit             (GtkAction       *action,
                                                             TerminalWindow     *window);

static void            terminal_window_close_window            (GtkAction *action, 
                                                             TerminalWindow     *window);
static void            terminal_window_action_show_full_screen (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_show_normal_screen(GtkToggleAction *action,
                                                              TerminalWindow     *window);
static void            terminal_window_set_toolbar (gboolean show);
static void            terminal_window_set_toolbar_fullscreen (gboolean show);
static void            terminal_window_select_all (GtkAction       *action,
                                                TerminalWindow     *window);
static void            terminal_widget_destroyed (GObject *obj, gpointer data);

/* Show toolbar */
static gboolean toolbar_fs = TRUE;
static gboolean toolbar_normal = TRUE;
static gboolean fs = FALSE;

struct _TerminalWindow
{
  HildonWindow __parent__;

  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  GtkWidget *menubar; /* menubar */
  GtkWidget *windows_menu; /* Where window menuitems are*/
  GtkAction *menuaction; /* Window menuitem */

  GConfClient *gconf_client;
  GSList              *keys;

  TerminalWidget *terminal;
  gchar *encoding;
};

static GObjectClass *parent_class;

static GtkActionEntry action_entries[] =
{

  { "file-menu", NULL, N_ ("File"), },
  { "open-url", NULL, N_ ("Open URL"), NULL, NULL, G_CALLBACK (terminal_window_open_url), },
  { "close-tab", NULL, N_ ("Close Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_close_tab), },
  { "shortcuts", NULL, N_ ("Shortcuts..."), NULL, NULL, G_CALLBACK (terminal_window_action_edit_shortcuts), },
  { "go-menu", NULL, N_ ("Go"), },
  { "font-menu", NULL, N_ ("Font size"), },
  { "terminal-menu", NULL, N_ ("Terminal"), },
  { "reset", NULL, N_ ("Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and Clear"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "ctrl", NULL, N_ ("Send Ctrl-<some key>"), NULL, NULL, G_CALLBACK (terminal_window_action_ctrl), },
  { "quit", NULL, N_ ("Quit"), NULL, NULL, G_CALLBACK (terminal_window_action_quit), },


  { "view-menu", NULL, ("webb_me_view"), },

  { "edit-menu", NULL, N_("weba_me_edit"), NULL, NULL, G_CALLBACK (terminal_window_action_edit), },
  { "copy", NULL, N_("weba_me_copy"), NULL, NULL, G_CALLBACK (terminal_window_action_copy), },
  { "paste", NULL, N_("weba_me_paste"), NULL, NULL, G_CALLBACK (terminal_window_action_paste), },
  { "tools-menu", NULL, N_("weba_me_tools"), },
  { "close-menu", NULL, N_("weba_me_close"), },

  { "windows-menu", NULL, ("weba_me_windows"), },
  { "new-window", NULL, N_("weba_me_new_window"), NULL, NULL, G_CALLBACK (terminal_window_action_new_window), }, 
  { "tools", NULL, N_("weba_me_tools"), },
  { "close-window", NULL, N_("weba_me_close_window"), NULL, NULL, G_CALLBACK (terminal_window_close_window), },
  { "close-all-windows", NULL, N_("weba_me_close_all_windows"), NULL, NULL, G_CALLBACK (terminal_window_action_quit), },

  { "show-toolbar-menu", NULL, N_("webb_me_show_toolbar"), },

  { "settings", NULL, N_("weba_me_settings"), NULL, NULL, G_CALLBACK (terminal_window_action_settings), },
  { "encoding", NULL, N_("weba_fi_encoding_method"), NULL, NULL, G_CALLBACK (terminal_window_action_encoding), },
#if 0
  { "curencoding", NULL, "", NULL, NULL, NULL, },
#endif
  { "select-all", NULL, ("weba_me_select_all"), NULL, NULL, G_CALLBACK (terminal_window_select_all), },

};

static GtkRadioActionEntry font_size_action_entries[] =
{
  { "-8pt", NULL, N_ ("-8 pt"), NULL, NULL, -8 },
  { "-6pt", NULL, N_ ("-6 pt"), NULL, NULL, -6 },
  { "-4pt", NULL, N_ ("-4 pt"), NULL, NULL, -4 },
  { "-2pt", NULL, N_ ("-2 pt"), NULL, NULL, -2 },
  { "+0pt", NULL, N_ ("+0 pt"), NULL, NULL, 0 },
  { "+2pt", NULL, N_ ("+2 pt"), NULL, NULL, +2 },
  { "+4pt", NULL, N_ ("+4 pt"), NULL, NULL, +4 },
  { "+6pt", NULL, N_ ("+6 pt"), NULL, NULL, +6 },
  { "+8pt", NULL, N_ ("+8 pt"), NULL, NULL, +8 },
};

static GtkToggleActionEntry toggle_action_entries[] =
{
  { "reverse", NULL, N_ ("Reverse colors"), NULL, NULL, G_CALLBACK (terminal_window_action_reverse), TRUE },
  { "scrollbar", NULL, N_ ("Scrollbar"), NULL, NULL, G_CALLBACK (terminal_window_action_scrollbar), TRUE },
  { "toolbar", NULL, N_ ("Toolbar"), NULL, NULL, G_CALLBACK (terminal_window_action_toolbar), TRUE },

  { "fullscreen", NULL, N_ ("webb_me_full_screen"), NULL, NULL, G_CALLBACK (terminal_window_action_fullscreen), FALSE },
  { "show-full-screen", NULL, N_ ("webb_me_full_screen"), NULL, NULL, G_CALLBACK (terminal_window_action_show_full_screen), TRUE},
  { "show-normal-screen", NULL, N_ ("webb_me_normal_mode"), NULL, NULL, G_CALLBACK(terminal_window_action_show_normal_screen), TRUE},
};

static const gchar ui_description[] =
 "<ui>"
 "  <popup name='popup-menu'>"
 "    <menuitem action='new-window'/>"
 "    <separator/>"
 "    <menuitem action='paste'/>"
 "  </popup>"
 "</ui>";

/*
 "    <separator/>"
 "    <menuitem action='settings'/>"
*/

G_DEFINE_TYPE (TerminalWindow, terminal_window, HILDON_TYPE_WINDOW);

typedef struct {
    GtkWidget *dialog;
    gchar *ret;
} ctrl_dialog_data;


static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GObjectClass *gobject_class;
  GType param_types;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_window_dispose;
  gobject_class->finalize = terminal_window_finalize;

  param_types = G_TYPE_POINTER;
  /* New window */
  terminal_window_signals[SIGNAL_NEW_WINDOW] =
            g_signal_newv ("new-window",
                           G_TYPE_FROM_CLASS (klass),
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                           G_SIGNAL_NO_HOOKS,
                           NULL,
                           NULL,
                           NULL,
                           g_cclosure_marshal_VOID__POINTER,
                           G_TYPE_NONE,
                           1, 
			   &param_types);
  /* For global state */
  terminal_window_signals[SIGNAL_WINDOW_STATE_CHANGED] =
            g_signal_newv ("global_state_changed",
                           G_TYPE_FROM_CLASS (klass),
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                           G_SIGNAL_NO_HOOKS,
                           NULL,
                           NULL,
                           NULL,
                           g_cclosure_marshal_VOID__VOID,
                           G_TYPE_NONE,
                           0,
                           NULL);


}

static GtkWidget *
attach_menu(GtkWidget *parent, GtkActionGroup *actiongroup,
            GtkAccelGroup *accelgroup, const gchar *name) 
{
    GtkAction *action;
    GtkWidget *menuitem, *menu;

    action = gtk_action_group_get_action(actiongroup, name);
    gtk_action_set_accel_group(action, accelgroup);
    menuitem = gtk_action_create_menu_item(action);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
    menu =  gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);

    return menu;
}

static void
attach_item(GtkWidget *parent, GtkActionGroup *actiongroup,
            GtkAccelGroup *accelgroup, const gchar *name) 
{
    GtkAction *action;
    GtkWidget *menuitem;

    action = gtk_action_group_get_action(actiongroup, name);
    gtk_action_set_accel_group(action, accelgroup);
    menuitem = gtk_action_create_menu_item(action);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
}

static void
populate_menubar (TerminalWindow *window, GtkAccelGroup *accelgroup)
{
  GtkActionGroup      *actiongroup = window->action_group;
  GtkWidget           *parent = NULL, *subparent;
  GtkWidget *menubar = NULL;

  menubar = gtk_menu_new();

  window->windows_menu = attach_menu(menubar, actiongroup, accelgroup, "windows-menu");

  parent = attach_menu(menubar, actiongroup, accelgroup, "edit-menu");
  attach_item(parent, actiongroup, accelgroup, "copy");
  attach_item(parent, actiongroup, accelgroup, "paste");
  /* TODO: ??? */
  /*  attach_item(parent, actiongroup, accelgroup, "select-all");
   */

  parent = attach_menu(menubar, actiongroup, accelgroup, "view-menu");
  attach_item(parent, actiongroup, accelgroup, "fullscreen");

  subparent = attach_menu(parent, actiongroup, accelgroup, "show-toolbar-menu");
  attach_item(subparent, actiongroup, accelgroup, "show-full-screen");
  attach_item(subparent, actiongroup, accelgroup, "show-normal-screen");

  parent = attach_menu(menubar, actiongroup, accelgroup, "tools-menu");
  attach_item(parent, actiongroup, accelgroup, "settings");
  attach_item(parent, actiongroup, accelgroup, "encoding");
#if 0
  attach_item(parent, actiongroup, accelgroup, "curencoding");
#endif

  parent = attach_menu(menubar, actiongroup, accelgroup, "close-menu");
  //  attach_item(parent, actiongroup, accelgroup, "close-window");
  attach_item(parent, actiongroup, accelgroup, "close-all-windows");
  
  hildon_window_set_menu(HILDON_WINDOW(window), GTK_MENU(menubar));

  attach_item(window->windows_menu, actiongroup, accelgroup, "new-window");
  window->menubar = menubar;
  gtk_widget_show_all(window->menubar);
}

static int
terminal_window_get_font_size(TerminalWindow *window) 
{
    GtkAction *action;

    action = gtk_action_group_get_action(window->action_group, "-8pt");
    if (!action) {
        return 0xf00b4; /* ?????? fooba(r) ????? */
    }

    return gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
}

static gboolean 
terminal_window_set_font_size(TerminalWindow *window, int new_size) 
{
    GtkAction *action;
    char new_name[5];

    if (new_size < -8 || new_size > 8) {
        return FALSE;
    }

    snprintf(new_name, sizeof(new_name), "%+dpt", new_size);

    action = gtk_action_group_get_action(window->action_group, new_name);
    if (!action) {
        return FALSE;
    }

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
    gtk_toggle_action_toggled(GTK_TOGGLE_ACTION(action));

    return TRUE;
}

static void
terminal_window_set_menu_normal (TerminalWindow *window, 
			      gboolean item_enabled)
{
  GtkAction *action = NULL;
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  action = gtk_action_group_get_action (window->action_group,
                                        "show-normal-screen");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), item_enabled);


}

static void
terminal_window_set_menu_fs (TerminalWindow *window,
			  gboolean item_enabled)
{
  GtkAction *action = NULL;
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  action = gtk_action_group_get_action (window->action_group,
                                        "show-full-screen");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), item_enabled);

}

static gboolean
terminal_window_set_menu_fs_idle (TerminalWindow *window)
{
  gboolean test = FALSE;
  GtkAction *action = NULL;
  action = gtk_action_group_get_action (window->action_group,
                                        "fullscreen");
  test = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  if (fs != test) {
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), fs);
  }

  return FALSE;
}

static gboolean
terminal_window_key_press_event (TerminalWindow *window,
                              GdkEventKey *event,
                              gpointer user_data) 
{
    int font_size;
    GtkAction *action;

    switch (event->keyval) 
    {
        case HILDON_HARDKEY_FULLSCREEN: /* Full screen */
            action = gtk_action_group_get_action(window->action_group,
                                                 "fullscreen");
            fs = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));

            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
                                         !fs);
	    g_signal_emit (G_OBJECT (window), 
			   terminal_window_signals[SIGNAL_WINDOW_STATE_CHANGED], 0);

            return TRUE;

        case HILDON_HARDKEY_INCREASE: /* Zoom in */
            font_size = terminal_window_get_font_size(window);
            if (font_size == 0xf00b4) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Getting font size failed!");
                return TRUE;
            }

            if (font_size >= 8) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Already at maximum font size.");
                return TRUE;
            }
            terminal_window_set_font_size(window, font_size + 2);
            return TRUE;

        case HILDON_HARDKEY_DECREASE: /* Zoom out */
            font_size = terminal_window_get_font_size(window);
            if (font_size == 0xf00b4) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Getting font size failed!");
                return TRUE;
            }
            
            if (font_size <= -8) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Already at minimum font size.");
                return TRUE;
            }
            terminal_window_set_font_size(window, font_size - 2);
            return TRUE;
	case HILDON_HARDKEY_HOME: /* Ignoring... */
	    return TRUE;
    }

    return FALSE;
}

#if 0
static void terminal_window_focus_in_event (TerminalWindow *window,
					       GdkEventFocus *event,
					       gpointer data)
{
}

static void terminal_window_focus_out_event (TerminalWindow *window,
					     GdkEventFocus *event,
					     gpointer data)
{
}
#endif

static void
terminal_window_init (TerminalWindow *window)
{
  GtkAction           *action;
  GtkAccelGroup       *accel_group;
  //  GtkWidget           *popup;
  GError              *error = NULL;
  gchar               *role;
  gint                 font_size;
  gboolean             scrollbar, toolbar, reverse;
  GConfValue          *gconf_value;
  GSList              *keys;
  GSList              *key_labels;

  window->terminal = NULL;
  window->encoding = NULL;

  gtk_window_set_title(GTK_WINDOW(window), "osso_xterm");
#if 0
  g_signal_connect (window, 
                    "focus-in-event", 
                    G_CALLBACK (terminal_window_focus_in_event), 
                    NULL);
  g_signal_connect (window, 
                    "focus-out-event", 
                    G_CALLBACK (terminal_window_focus_out_event), 
                    NULL);
#endif
  g_signal_connect( G_OBJECT(window), "key-press-event",
                    G_CALLBACK(terminal_window_key_press_event), NULL);
  window->gconf_client = gconf_client_get_default();
  
  font_size = gconf_client_get_int(window->gconf_client,
                                   OSSO_XTERM_GCONF_FONT_SIZE,
                                   &error);
  if (error != NULL) {
      g_printerr("Unable to get font size from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_SCROLLBAR,
                                 &error);

  if (error != NULL) {
      g_printerr("Unable to get scrollbar setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }
  scrollbar = OSSO_XTERM_DEFAULT_SCROLLBAR;
  if (gconf_value) {
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  scrollbar = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_TOOLBAR,
                                 &error);
  if (error != NULL) {
      g_printerr("Unable to get toolbar setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }
  toolbar = OSSO_XTERM_DEFAULT_TOOLBAR;
  if (gconf_value) {
      if (gconf_value->type == GCONF_VALUE_BOOL) {
          toolbar = gconf_value_get_bool(gconf_value);
      }
      gconf_value_free(gconf_value);
  }
  toolbar_normal = toolbar;

  /* toolbar to fullscreen */
  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_TOOLBAR_FULLSCREEN,
                                 &error);
  if (error != NULL) {
      g_printerr("Unable to get toolbar setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }
  toolbar = OSSO_XTERM_DEFAULT_TOOLBAR_FULLSCREEN;
  if (gconf_value) {
      if (gconf_value->type == GCONF_VALUE_BOOL) {
          toolbar = gconf_value_get_bool(gconf_value);
      }
      gconf_value_free(gconf_value);
  }
  toolbar_fs = toolbar;

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_REVERSE,
                                 &error);
  if (error != NULL) {
      g_printerr("Unable to get reverse setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }
  reverse = OSSO_XTERM_DEFAULT_REVERSE;
  if (gconf_value) {
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  reverse = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  window->encoding = gconf_client_get_string (window->gconf_client, 
					      OSSO_XTERM_GCONF_ENCODING,
					      &error);
  if (error != NULL) {
      g_printerr("Unable to get encoding setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }

  window->action_group = gtk_action_group_new ("terminal-window");
  gtk_action_group_set_translation_domain (window->action_group,
                                           GETTEXT_PACKAGE);
  gtk_action_group_add_actions (window->action_group,
                                action_entries,
                                G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (window));
  gtk_action_group_add_toggle_actions (window->action_group,
                                       toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries),
                                       GTK_WIDGET (window));

  gtk_action_group_add_radio_actions (window->action_group,
                                      font_size_action_entries,
                                      G_N_ELEMENTS(font_size_action_entries),
                                      font_size,
                                      G_CALLBACK(terminal_window_action_font_size),
                                      GTK_WIDGET(window));;

  action = gtk_action_group_get_action(window->action_group, "scrollbar");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), scrollbar);

  action = gtk_action_group_get_action(window->action_group, "toolbar");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE); //toolbar);

  action = gtk_action_group_get_action(window->action_group, "show-full-screen");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar_fs);
  action = gtk_action_group_get_action(window->action_group, "show-normal-screen");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar_normal);

  action = gtk_action_group_get_action(window->action_group, "reverse");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), reverse);

  window->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (window->ui_manager, window->action_group, 0);
  if (gtk_ui_manager_add_ui_from_string (window->ui_manager,
                                         ui_description, strlen(ui_description),
                                         &error) == 0)
  {
      g_warning ("Unable to create menu: %s", error->message);
      g_error_free (error);
  }

  accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  populate_menubar(window, accel_group);

  /*
  popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
  gtk_widget_tap_and_hold_setup(GTK_WIDGET(window), popup, NULL,
                                GTK_TAP_AND_HOLD_NONE);
  */
  
  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      action = gtk_action_group_get_action (window->action_group, "fullscreen");
      g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
    }

  keys = gconf_client_get_list(window->gconf_client,
                                 OSSO_XTERM_GCONF_KEYS,
                                 GCONF_VALUE_STRING,
                                 &error);
  key_labels = gconf_client_get_list(window->gconf_client,
                                 OSSO_XTERM_GCONF_KEY_LABELS,
                                 GCONF_VALUE_STRING,
                                 &error);

  g_slist_foreach(keys, (GFunc)g_free, NULL);
  g_slist_foreach(key_labels, (GFunc)g_free, NULL);
  g_slist_free(keys);
  g_slist_free(key_labels);

  /* set a unique role on each window (for session management) */
  role = g_strdup_printf ("Terminal-%p-%d-%d", window, getpid (), (gint) time (NULL));
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);

}


static void
terminal_window_dispose (GObject *object)
{
  parent_class->dispose (object);
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  if (window->terminal != NULL)
    g_object_unref (window->terminal);
  if (window->action_group != NULL)
    g_object_unref (G_OBJECT (window->action_group));
  if (window->ui_manager != NULL)
    g_object_unref (G_OBJECT (window->ui_manager));


  g_object_unref(G_OBJECT(window->gconf_client));

  parent_class->finalize (object);
}


static TerminalWidget*
terminal_window_get_active (TerminalWindow *window)
{
  return TERMINAL_WIDGET (window->terminal);    
}

static void
terminal_window_update_actions (TerminalWindow *window)
{
  TerminalWidget *terminal;
  GtkAction      *action;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      action = gtk_action_group_get_action (window->action_group, "copy");
      g_object_set (G_OBJECT (action),
                    "sensitive", terminal_widget_has_selection (terminal),
                    NULL);
    }
}

static void
terminal_window_notify_title(TerminalWidget *terminal,
                          GParamSpec   *pspec,
                          TerminalWindow  *window)
{
  TerminalWidget *active_terminal;
  gchar *terminal_title = terminal_widget_get_title(terminal);
  active_terminal = terminal_window_get_active (window);
  if (G_LIKELY( terminal == active_terminal )) {
    gtk_window_set_title(GTK_WINDOW(window), terminal_title);
  }
  g_free(terminal_title);
}

static void
terminal_window_context_menu (TerminalWidget  *widget,
                           GdkEvent        *event,
                           TerminalWindow     *window)
{

/* Copy & paste didn't work quite well from popup menu and there was only one 
 * item left in the menu so removed
 */
#if 0
  TerminalWidget *terminal;
  //  GtkWidget      *popup;
  gint            button = 0;
  gint            time;

  terminal = terminal_window_get_active (window);

  if (G_LIKELY (widget == terminal))
    {
      
      popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
      gtk_widget_show_all(GTK_WIDGET(popup));
      

      if (G_UNLIKELY (popup == NULL))
        return;

      if (event != NULL)
        {
          if (event->type == GDK_BUTTON_PRESS)
            {
	      char *msg = terminal_widget_get_tag(terminal,
			      event->button.x,
			      event->button.y,
			      NULL);
		  GtkWidget *item = gtk_ui_manager_get_widget(window->ui_manager,
				  "/popup-menu/open-url");
	      if (msg == NULL)
	        {
		  gtk_widget_hide(item);
		  g_object_set_data(G_OBJECT(window), "url", NULL);
		}
	      else
	        {
		  gtk_widget_show(item);
		  g_object_set_data_full(G_OBJECT(window), "url", msg, g_free);
		}
              button = event->button.button;
	    }
	    time = event->button.time;
        }
      else
        {
          time = gtk_get_current_event_time ();
        }

      terminal->im_pending = FALSE;

      gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                      NULL, NULL, button, time);
    }
#endif
}

static void
terminal_window_open_url (GtkAction	*action,
		       TerminalWindow	*window)
{
  osso_context_t *osso = (osso_context_t *)g_object_get_data(
		  G_OBJECT(window), "osso");
  gchar *url = (gchar *)g_object_get_data(G_OBJECT(window), "url");

  if (url && osso) {
    osso_rpc_run_with_defaults(osso,
		    "osso_browser",
		    OSSO_BROWSER_OPEN_NEW_WINDOW_REQ,
		    NULL,
		    DBUS_TYPE_STRING,
		    url,
		    DBUS_TYPE_INVALID);
  }
  g_object_set_data(G_OBJECT(window), "url", NULL);
}


static void
terminal_window_action_new_window (GtkAction    *action,
                             TerminalWindow  *window)
{

  /* new-window signal */
  g_signal_emit (G_OBJECT (window), 
                 terminal_window_signals[SIGNAL_NEW_WINDOW], 0, NULL); 

}

static void
terminal_window_action_close_tab (GtkAction    *action,
                               TerminalWindow  *window)
{
  gtk_widget_hide (GTK_WIDGET (window));
  gtk_widget_destroy (GTK_WIDGET (window));

}

/* Check is paste enabled */
static void
terminal_window_action_edit (GtkAction *action,
                             TerminalWindow *window)
{
  GtkAction *pasteaction;
  GdkAtom target = gdk_atom_intern ("CLIPBOARD", FALSE);
  GtkClipboard *clipboard = gtk_clipboard_get (target);
  gboolean paste_enabled = gtk_clipboard_wait_is_text_available (clipboard);

  pasteaction = gtk_action_group_get_action (window->action_group, "paste");
  if (pasteaction != NULL) {
    g_object_set (G_OBJECT (pasteaction), "sensitive", paste_enabled, NULL);
  }
}


static void
terminal_window_action_copy (GtkAction    *action,
                          TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_copy_clipboard (terminal);
}


static void
terminal_window_action_paste (GtkAction    *action,
                           TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
}

static void
terminal_window_action_edit_shortcuts (GtkAction    *action,
                           TerminalWindow  *window)
{
  (void)action;
  (void)window;

//  update_shortcut_keys();
}


static void
terminal_window_action_reverse (GtkToggleAction *action,
                             TerminalWindow     *window)
{
    GConfClient *client;
    gboolean reverse;

    client = gconf_client_get_default();
    reverse = gtk_toggle_action_get_active (action);

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_REVERSE,
                           reverse,
                           NULL);

    g_object_unref(G_OBJECT(client));
}


static void
terminal_window_action_fullscreen (GtkToggleAction *action,
                                TerminalWindow     *window)
{
  fs = gtk_toggle_action_get_active (action);

  g_signal_emit (G_OBJECT (window), 
		   terminal_window_signals[SIGNAL_WINDOW_STATE_CHANGED], 0);

}

static void
terminal_window_set_toolbar (gboolean show)
{
    GConfClient *client;
    client = gconf_client_get_default();

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_TOOLBAR,
                           show,
                           NULL);

    g_object_unref(G_OBJECT(client));
}

static void
terminal_window_set_toolbar_fullscreen (gboolean show)
{
    GConfClient *client;
    client = gconf_client_get_default();

#ifdef DEBUG
    g_debug ("%s - %s", __FUNCTION__, show==TRUE?"TRUE":"FALSE");
#endif

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_TOOLBAR_FULLSCREEN,
                           show,
                           NULL);

    g_object_unref(G_OBJECT(client));
}

static void
terminal_window_action_toolbar (GtkToggleAction *action,
                               TerminalWindow     *window)
{
    gboolean show;

    show = gtk_toggle_action_get_active (action);
    terminal_window_set_toolbar (show);

}

static void
terminal_window_action_scrollbar (GtkToggleAction *action,
                               TerminalWindow     *window)
{
    GConfClient *client;
    gboolean show;

    client = gconf_client_get_default();
    show = gtk_toggle_action_get_active (action);

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_SCROLLBAR,
                           show,
                           NULL);

    g_object_unref(G_OBJECT(client));
}


static void
terminal_window_action_font_size (GtkRadioAction *action,
                               GtkRadioAction *current,
                               TerminalWindow    *window)
{
    GConfClient *client;
    gint size;

    client = gconf_client_get_default();
    size = gtk_radio_action_get_current_value(current);

    gconf_client_set_int (client,
                          OSSO_XTERM_GCONF_FONT_SIZE,
                          size,
                          NULL);

    g_object_unref(G_OBJECT(client));
}


static void
terminal_window_action_reset (GtkAction   *action,
                           TerminalWindow *window)
{
  TerminalWidget *active;

  active = terminal_window_get_active (window);
  terminal_widget_reset (active, FALSE);
}


static void
terminal_window_action_reset_and_clear (GtkAction    *action,
                                     TerminalWindow  *window)
{
  TerminalWidget *active;

  active = terminal_window_get_active (window);
  terminal_widget_reset (active, TRUE);
}

static void
terminal_window_send_ctrl_key(TerminalWindow *window,
                           const char *str)
{
  GdkEventKey *key;
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif

  key = (GdkEventKey *) gdk_event_new(GDK_KEY_PRESS);

  key->window = GTK_WIDGET(window)->window;
  key->time = GDK_CURRENT_TIME;
  key->state = GDK_CONTROL_MASK;
  key->keyval = gdk_keyval_from_name(str);
  gdk_event_put ((GdkEvent *) key);

  key->type = GDK_KEY_RELEASE;
  key->state |= GDK_RELEASE_MASK;
  gdk_event_put ((GdkEvent *) key);

  gdk_event_free((GdkEvent *) key);
}

static gboolean ctrl_dialog_focus(GtkWidget *dialog,
                                  GdkEventFocus *event,
                                  GtkIMContext *imctx)
{
  if (event->in) {
    gtk_im_context_focus_in(imctx);
    gtk_im_context_show(imctx);
  } else {
    gtk_im_context_focus_out(imctx);
  }
  return FALSE;
}

static gboolean
im_context_commit (GtkIMContext *ctx,
                   const gchar *str,
                   ctrl_dialog_data *data)
{
#ifdef DEBUG
    g_debug (__FUNCTION__);
#endif
    if (strlen(str) > 0) {
      data->ret = g_strdup(str);
      gtk_dialog_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_ACCEPT);
    }

    return TRUE;
}

static void
terminal_window_action_ctrl (GtkAction    *action,
                             TerminalWindow  *window)
{
  ctrl_dialog_data *data;
  GtkWidget *dialog, *label;
  GtkIMContext *imctx;
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif

  dialog = gtk_dialog_new_with_buttons("Control",
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       "Cancel", GTK_RESPONSE_CANCEL,
                                       NULL);

  imctx = gtk_im_multicontext_new();

  data = g_new0(ctrl_dialog_data, 1);
  data->dialog = dialog;
  g_signal_connect(imctx, "commit", G_CALLBACK(im_context_commit), data);

  label = gtk_label_new("Press a key (or cancel)");
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), label);

  gtk_widget_show_all(dialog);

  gtk_im_context_set_client_window(imctx, GTK_WIDGET(dialog)->window);

  g_signal_connect( G_OBJECT(dialog), "focus-in-event",
          G_CALLBACK(ctrl_dialog_focus), imctx);
  g_signal_connect( G_OBJECT(dialog), "focus-out-event",
          G_CALLBACK(ctrl_dialog_focus), imctx);

  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_hide(dialog);
  gtk_widget_destroy(dialog);

  gtk_im_context_focus_out(imctx);
  g_object_unref(G_OBJECT(imctx));

  if (data->ret != NULL) {
    terminal_window_send_ctrl_key(window, data->ret);
    g_free(data->ret);
  }

  g_free(data);
}

static void
terminal_window_action_encoding (GtkAction       *action,
				 TerminalWindow  *window)
{
  gchar *retval = NULL;
  gchar *encoding = NULL;
  g_object_get (window->terminal, "encoding", &encoding, NULL);
  retval = terminal_encoding_dialog (window->terminal, GTK_WINDOW (window), 
				     encoding);
#ifdef DEBUG
  g_debug ("%s - retval: %s",__FUNCTION__, retval);
#endif

  g_free (retval);


}

static void
terminal_window_action_settings (GtkAction    *action,
                              TerminalWindow  *window)
{
    GtkWidget *settings;
    
    settings = terminal_settings_new();
    gtk_window_set_transient_for (GTK_WINDOW (settings), GTK_WINDOW (window));

    switch (gtk_dialog_run(GTK_DIALOG(settings))) {
        case GTK_RESPONSE_OK:
	  terminal_settings_store(TERMINAL_SETTINGS(settings), 
				  window->terminal);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default:
            break;
    }
    gtk_widget_hide (settings);
    gtk_widget_destroy (settings);
}


static void
terminal_window_action_quit (GtkAction    *action,
                          TerminalWindow  *window)
{
  //  gtk_widget_destroy(GTK_WIDGET(window));
  gtk_main_quit ();
}


/**
 * terminal_window_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_window_new (void)
{
  return g_object_new (TERMINAL_TYPE_WINDOW, NULL);
}

static void 
terminal_widget_destroyed (GObject *obj, gpointer data)
{
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  g_assert (TERMINAL_IS_WINDOW (data));
  TERMINAL_WINDOW (data)->terminal = NULL;
  gtk_widget_destroy (GTK_WIDGET (data));

}

static void
terminal_window_real_add (TerminalWindow    *window,
		       TerminalWidget *widget)
{
  /*
    gchar *title = terminal_widget_get_title(widget);
    g_object_set(GTK_WINDOW (window), "title", title, NULL);
    g_free(title);
  */
    gtk_container_add(GTK_CONTAINER (window), GTK_WIDGET(widget));

    /* add terminal to window */
    TERMINAL_WINDOW (window)->terminal = widget;

    g_signal_connect(G_OBJECT(widget), "notify::title",
  		    G_CALLBACK(terminal_window_notify_title), window);
    g_signal_connect (G_OBJECT (widget), "context-menu",
                      G_CALLBACK (terminal_window_context_menu), window);
    g_signal_connect (G_OBJECT (widget), "destroy",
                      G_CALLBACK (terminal_widget_destroyed), window);
    g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                              G_CALLBACK (terminal_window_update_actions), window);
    terminal_window_update_actions (window);
    /*   window_list = g_slist_append(window_list, window);
     */
}

/**
 * terminal_window_add:
 * @window    : A #TerminalWindow.
 * @widget : A #TerminalWidget.
 **/
GtkWidget *
terminal_window_add (TerminalWindow    *window,
                  TerminalWidget *widget)
{
  gpointer newwindow = NULL;
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  gtk_widget_show (GTK_WIDGET (widget));

  terminal_window_real_add (window, widget);

  if (GTK_IS_WINDOW (newwindow) == TRUE) {
    if (!fs) {
      gtk_window_unfullscreen(GTK_WINDOW(newwindow));
    } else {
      gtk_window_fullscreen(GTK_WINDOW(newwindow));
    }
  }

  return newwindow;
}


/**
 * terminal_window_remove:
 * @window     :
 * @widget  :
 **/
void
terminal_window_remove (TerminalWindow    *window,
                     TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  terminal_window_set_toolbar (toolbar_normal);
  terminal_window_set_toolbar_fullscreen (toolbar_fs);

  gtk_widget_destroy (GTK_WIDGET (widget));
}

static gboolean 
_im_context_focus (TerminalWidget *terminal)
{
  /* Keep IM open on startup */
  hildon_gtk_im_context_show(terminal->im_context);
  gtk_im_context_focus_in (terminal->im_context);
  
  return FALSE;
}

/**
 * terminal_window_launch
 * @window         : A #TerminalWindow.
 * @error       : Location to store error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_window_launch (TerminalWindow     *window,
		     const gchar     *command,
                     GError          **error)
{
  GtkWidget *terminal;

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();
  terminal_widget_set_working_directory(TERMINAL_WIDGET(terminal),
		 g_get_home_dir());
  terminal_widget_set_app_win (TERMINAL_WIDGET (terminal), HILDON_WINDOW (window));

  (void *)terminal_window_add (window, TERMINAL_WIDGET (terminal));
  if (command) {
    gint argc;
    gchar **argv;

    if (g_shell_parse_argv(command,
	  &argc,
	  &argv,
	  NULL)) {
      terminal_widget_set_custom_command(TERMINAL_WIDGET (terminal),
	  argv);
      g_strfreev(argv);
    }
  }
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

  gtk_widget_show_all(GTK_WIDGET(window));

  if (window->encoding == NULL) {
    gconf_client_set_string(window->gconf_client, OSSO_XTERM_GCONF_ENCODING, 
			    OSSO_XTERM_DEFAULT_ENCODING, NULL);
    g_object_set (window->terminal, "encoding", 
		  OSSO_XTERM_DEFAULT_ENCODING, NULL);
  } else {
    g_object_set (window->terminal, "encoding", window->encoding, NULL);
  }

  g_idle_add ((GSourceFunc)_im_context_focus, window->terminal);

  return TRUE;
}

static void
terminal_window_close_window(GtkAction *action, TerminalWindow *window)
{
  g_assert (window);
  g_assert (TERMINAL_IS_WINDOW (window));

  gtk_widget_destroy (GTK_WIDGET (window));
}

static void            
terminal_window_action_show_full_screen (GtkToggleAction *action,
                                      TerminalWindow     *window)
{
    toolbar_fs = gtk_toggle_action_get_active (action);
    terminal_window_set_toolbar_fullscreen (toolbar_fs);  

    g_signal_emit (G_OBJECT (window), 
		   terminal_window_signals[SIGNAL_WINDOW_STATE_CHANGED], 0);

}

static void
terminal_window_action_show_normal_screen(GtkToggleAction *action,
                                       TerminalWindow     *window)
{
    toolbar_normal = gtk_toggle_action_get_active (action);
    terminal_window_set_toolbar (toolbar_normal);

    g_signal_emit (G_OBJECT (window), 
		   terminal_window_signals[SIGNAL_WINDOW_STATE_CHANGED], 0);
}

static void
terminal_window_select_all (GtkAction    *action,
                              TerminalWindow  *window)
{
#ifdef DEBUG
    g_debug(__FUNCTION__);
#endif
    g_assert (window != NULL);
    g_assert (TERMINAL_IS_WINDOW (window));
    /* terminal_widget_select_all (TERMINAL_WIDGET (window->terminal)); */
}

void terminal_window_set_state (TerminalWindow *window, TerminalWindow *current)
{
#ifdef DEBUG
    g_debug ("%s : tb_normal: %d - tb_fs: %d", 
	   __FUNCTION__, toolbar_normal, toolbar_fs);
#endif
    terminal_window_set_menu_normal (window, toolbar_normal);
    terminal_window_set_menu_fs (window, toolbar_fs);

    if (!fs) {
      if (window == current) {
	gtk_window_unfullscreen(GTK_WINDOW(window));
	g_idle_add ((GSourceFunc)terminal_window_set_menu_fs_idle, window);
      }
      terminal_window_set_toolbar (toolbar_normal);
      terminal_widget_update_tool_bar(window->terminal, toolbar_normal);
    } else {
      if (window == current) {
	gtk_window_fullscreen(GTK_WINDOW(window));
	g_idle_add ((GSourceFunc)terminal_window_set_menu_fs_idle, window);
      }
      terminal_window_set_toolbar_fullscreen (toolbar_fs);
      terminal_widget_update_tool_bar(window->terminal, toolbar_fs);
    }

}
