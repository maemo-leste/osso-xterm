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
#include <osso-browser-interface.h>

#include <libintl.h>
#include <locale.h>
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
#include "terminal-app.h"
#include "shortcuts.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

static void            terminal_app_dispose                 (GObject         *object);
static void            terminal_app_finalize                (GObject         *object);
#ifdef USE_NOTEBOOK
static TerminalWidget *terminal_app_get_active              (TerminalApp     *app);
static void            terminal_app_set_size_force_grid     (TerminalApp     *app,
                                                             TerminalWidget  *widget,
                                                             gint             force_grid_width,
                                                             gint             force_grid_height);
static void            terminal_app_update_geometry         (TerminalApp     *app);
static void            terminal_app_update_actions          (TerminalApp     *app);
static void            terminal_app_notify_page             (GtkNotebook     *notebook,
                                                             GParamSpec      *pspec,
                                                             TerminalApp     *app);
static void            terminal_app_notify_title            (TerminalWidget  *terminal,
                                                             GParamSpec      *pspec,
                                                             TerminalApp     *app);
static void            terminal_app_context_menu            (TerminalWidget  *widget,
                                                             GdkEvent        *event,
                                                             TerminalApp     *app);
static void            terminal_app_widget_removed          (GtkNotebook     *notebook,
                                                             TerminalWidget  *widget,
                                                             TerminalApp     *app);
#endif

static void            terminal_app_open_url                (GtkAction       *action,
		                                             TerminalApp     *app);
static void            terminal_app_action_new_tab          (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_close_tab        (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_copy             (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_paste            (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_edit_shortcuts   (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_reverse          (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_fullscreen       (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_scrollbar        (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_toolbar        (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_prev_tab         (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_next_tab         (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_font_size        (GtkRadioAction  *action,
                                                             GtkRadioAction  *current,
                                                             TerminalApp     *app);
static void            terminal_app_action_reset            (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_reset_and_clear  (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_ctrl             (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_settings         (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_quit             (GtkAction       *action,
                                                             TerminalApp     *app);
#ifndef USE_NOTEBOOK
static void            terminal_app_close_window            (GtkWidget *widget, 
                                                             gpointer data);

/* This keeps count of number of windows */
static gint windows = 0;
#endif

struct _TerminalApp
{
  HildonWindow         __parent__;

  GtkActionGroup      *action_group;
  GtkUIManager        *ui_manager;
#if USE_NOTEBOOK
  GtkWidget           *notebook;
#endif
};


static GObjectClass *parent_class;

static GtkActionEntry action_entries[] =
{

  { "file-menu", NULL, N_ ("File"), },
  { "windows-menu", NULL, N_ ("Windows"), },
  { "open-url", NULL, N_ ("Open URL"), NULL, NULL, G_CALLBACK (terminal_app_open_url), },
  { "new-window", NULL, N_ ("New window"), NULL, NULL, G_CALLBACK (terminal_app_action_new_tab), }, 
  { "new-tab", NULL, N_ ("New Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_new_tab), }, 
  { "close-tab", NULL, N_ ("Close Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_close_tab), },
  { "edit-menu", NULL, N_ ("Edit"),  },
  { "copy", NULL, N_ ("Copy"), NULL, NULL, G_CALLBACK (terminal_app_action_copy), },
  { "paste", NULL, N_ ("Paste"), NULL, NULL, G_CALLBACK (terminal_app_action_paste), },
  { "shortcuts", NULL, N_ ("Shortcuts..."), NULL, NULL, G_CALLBACK (terminal_app_action_edit_shortcuts), },
  { "view-menu", NULL, N_ ("View"), },
  { "go-menu", NULL, N_ ("Go"), },
  { "prev-tab", NULL, N_ ("Previous Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_prev_tab), },
  { "next-tab", NULL, N_ ("Next Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_next_tab), },
#if 0
  { "font-menu", NULL, N_ ("Font size"), },
#endif
  { "terminal-menu", NULL, N_ ("Terminal"), },
  { "reset", NULL, N_ ("Reset"), NULL, NULL, G_CALLBACK (terminal_app_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and Clear"), NULL, NULL, G_CALLBACK (terminal_app_action_reset_and_clear), },
  { "ctrl", NULL, N_ ("Send Ctrl-<some key>"), NULL, NULL, G_CALLBACK (terminal_app_action_ctrl), },
  { "settings", NULL, N_ ("Settings..."), NULL, NULL, G_CALLBACK (terminal_app_action_settings), },
  { "quit", NULL, N_ ("Quit"), NULL, NULL, G_CALLBACK (terminal_app_action_quit), },
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
  { "reverse", NULL, N_ ("Reverse colors"), NULL, NULL, G_CALLBACK (terminal_app_action_reverse), TRUE },
  { "fullscreen", NULL, N_ ("Fullscreen"), NULL, NULL, G_CALLBACK (terminal_app_action_fullscreen), FALSE },
  { "scrollbar", NULL, N_ ("Scrollbar"), NULL, NULL, G_CALLBACK (terminal_app_action_scrollbar), TRUE },
  { "toolbar", NULL, N_ ("Toolbar"), NULL, NULL, G_CALLBACK (terminal_app_action_toolbar), TRUE },
};

static const gchar ui_description[] =
 "<ui>"
 "  <popup name='popup-menu'>"
 "    <menuitem action='open-url'/>"
 "    <separator/>"
 "    <menuitem action='new-tab'/>"
 "    <menuitem action='close-tab'/>"
 "    <separator/>"
 "    <menuitem action='copy'/>"
 "    <menuitem action='paste'/>"
 "    <menuitem action='shortcuts'/>"
 "    <separator/>"
 "    <menuitem action='reverse'/>"
 "    <menuitem action='scrollbar'/>"
 "    <menuitem action='toolbar'/>"
 "    <menuitem action='fullscreen'/>"
 "    <separator/>"
 "    <menuitem action='ctrl'/>"
#if 0
 "    <separator/>"
 "    <menu action='font-menu'>"
 "      <menuitem action='-8pt'/>"
 "      <menuitem action='-6pt'/>"
 "      <menuitem action='-4pt'/>"
 "      <menuitem action='-2pt'/>"
 "      <menuitem action='+0pt'/>"
 "      <menuitem action='+2pt'/>"
 "      <menuitem action='+4pt'/>"
 "      <menuitem action='+6pt'/>"
 "      <menuitem action='+8pt'/>"
 "    </menu>"
#endif
 "  </popup>"
 "</ui>";


G_DEFINE_TYPE (TerminalApp, terminal_app, HILDON_TYPE_WINDOW);


typedef struct {
    GtkWidget *dialog;
    gchar *ret;
} ctrl_dialog_data;


static void
terminal_app_class_init (TerminalAppClass *klass)
{
  GObjectClass *gobject_class;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_app_dispose;
  gobject_class->finalize = terminal_app_finalize;
}

static GtkWidget *
attach_menu(GtkWidget *parent, GtkActionGroup *actiongroup,
            GtkAccelGroup *accelgroup, const gchar *name) {
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
            GtkAccelGroup *accelgroup, const gchar *name) {
    GtkAction *action;
    GtkWidget *menuitem;

    action = gtk_action_group_get_action(actiongroup, name);
    gtk_action_set_accel_group(action, accelgroup);
    menuitem = gtk_action_create_menu_item(action);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
}


static void
populate_menubar (TerminalApp *app, GtkAccelGroup *accelgroup)
{
  GtkWidget           *menubar, *parent;
  GtkActionGroup      *actiongroup = app->action_group;

  menubar = gtk_menu_new();

#ifdef USE_NOTEBOOK
  parent = attach_menu(menubar, actiongroup, accelgroup, "file-menu");
  attach_item(parent, actiongroup, accelgroup, "new-tab");
  attach_item(parent, actiongroup, accelgroup, "close-tab");
#else
  parent = attach_menu(menubar, actiongroup, accelgroup, "windows-menu");
  attach_item(parent, actiongroup, accelgroup, "new-window");
#endif

  parent = attach_menu(menubar, actiongroup, accelgroup, "edit-menu");
  attach_item(parent, actiongroup, accelgroup, "copy");
  attach_item(parent, actiongroup, accelgroup, "paste");
  attach_item(parent, actiongroup, accelgroup, "shortcuts");

  parent = attach_menu(menubar, actiongroup, accelgroup, "view-menu");
  attach_item(parent, actiongroup, accelgroup, "reverse");
  attach_item(parent, actiongroup, accelgroup, "scrollbar");
  attach_item(parent, actiongroup, accelgroup, "toolbar");
  attach_item(parent, actiongroup, accelgroup, "fullscreen");
  gtk_menu_shell_append(GTK_MENU_SHELL(parent),
                        gtk_separator_menu_item_new());
#if 0
  parent = attach_menu(parent, actiongroup, accelgroup, "font-menu");
  attach_item(parent, actiongroup, accelgroup, "-8pt");
  attach_item(parent, actiongroup, accelgroup, "-6pt");
  attach_item(parent, actiongroup, accelgroup, "-4pt");
  attach_item(parent, actiongroup, accelgroup, "-2pt");
  attach_item(parent, actiongroup, accelgroup, "+0pt");
  attach_item(parent, actiongroup, accelgroup, "+2pt");
  attach_item(parent, actiongroup, accelgroup, "+4pt");
  attach_item(parent, actiongroup, accelgroup, "+6pt");
  attach_item(parent, actiongroup, accelgroup, "+8pt");
#endif

  parent = attach_menu(menubar, actiongroup, accelgroup, "go-menu");
  attach_item(parent, actiongroup, accelgroup, "prev-tab");
  attach_item(parent, actiongroup, accelgroup, "next-tab");

  parent = attach_menu(menubar, actiongroup, accelgroup, "terminal-menu");
  attach_item(parent, actiongroup, accelgroup, "reset");
  attach_item(parent, actiongroup, accelgroup, "reset-and-clear");
  attach_item(parent, actiongroup, accelgroup, "ctrl");

  attach_item(menubar, actiongroup, accelgroup, "settings");

  attach_item(menubar, actiongroup, accelgroup, "quit");

  gtk_widget_show_all(menubar);

  hildon_window_set_menu(HILDON_WINDOW(app), GTK_MENU(menubar));
}

static int
terminal_app_get_font_size(TerminalApp *app) {
    GtkAction *action;

    action = gtk_action_group_get_action(app->action_group, "-8pt");
    if (!action) {
        return 0xf00b4;
    }

    return gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
}

static
gboolean terminal_app_set_font_size(TerminalApp *app, int new_size) {
    GtkAction *action;
    char new_name[5];

    if (new_size < -8 || new_size > 8) {
        return FALSE;
    }

    snprintf(new_name, sizeof(new_name), "%+dpt", new_size);

    action = gtk_action_group_get_action(app->action_group, new_name);
    if (!action) {
        return FALSE;
    }

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
    gtk_toggle_action_toggled(GTK_TOGGLE_ACTION(action));

    return TRUE;
}


static gboolean
terminal_app_key_press_event (TerminalApp *app,
                              GdkEventKey *event,
                              gpointer user_data) {
    gboolean fs;
    int font_size;
    GtkAction *action;

    switch (event->keyval) 
    {
        case HILDON_HARDKEY_FULLSCREEN: /* Full screen */
            action = gtk_action_group_get_action(app->action_group,
                                                 "fullscreen");
            fs = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	    if (fs) {
		gtk_window_unfullscreen(GTK_WINDOW(app));
	    } else {
		gtk_window_fullscreen(GTK_WINDOW(app));
	    }
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
                                         !fs);
            return TRUE;

        case HILDON_HARDKEY_INCREASE: /* Zoom in */
            font_size = terminal_app_get_font_size(app);
            if (font_size == 0xf00b4) {
		hildon_banner_show_information(GTK_WIDGET(app), NULL,
			"Getting font size failed!");
                return TRUE;
            }

            if (font_size >= 8) {
		hildon_banner_show_information(GTK_WIDGET(app), NULL,
			"Already at maximum font size.");
                return TRUE;
            }
            terminal_app_set_font_size(app, font_size + 2);
            return TRUE;

        case HILDON_HARDKEY_DECREASE: /* Zoom out */
            font_size = terminal_app_get_font_size(app);
            if (font_size == 0xf00b4) {
		hildon_banner_show_information(GTK_WIDGET(app), NULL,
			"Getting font size failed!");
                return TRUE;
            }
            
            if (font_size <= -8) {
		hildon_banner_show_information(GTK_WIDGET(app), NULL,
			"Already at minimum font size.");
                return TRUE;
            }
            terminal_app_set_font_size(app, font_size - 2);
            return TRUE;
	case HILDON_HARDKEY_HOME: /* Ignoring... */
	    return TRUE;
    }

    return FALSE;
}

static void
terminal_app_init (TerminalApp *app)
{
  GtkAction           *action;
  GtkAccelGroup       *accel_group;
#if USE_NOTEBOOK
  GtkWidget           *vbox;
#endif
  GtkWidget           *popup;
  GError              *error = NULL;
  gchar               *role;
  gint                 font_size;
  gboolean             scrollbar, toolbar, reverse;
  GConfClient         *gconf_client;
  GConfValue          *gconf_value;
  HildonProgram       *program;

  program = hildon_program_get_instance();
  hildon_program_add_window(program, HILDON_WINDOW(app));

  gtk_window_set_title(GTK_WINDOW(app), "osso_xterm");

  g_signal_connect( G_OBJECT(app), "key-press-event",
                    G_CALLBACK(terminal_app_key_press_event), NULL);

  gconf_client = gconf_client_get_default();
  
  font_size = gconf_client_get_int(gconf_client,
                                   OSSO_XTERM_GCONF_FONT_SIZE,
                                   &error);
  if (error != NULL) {
      g_printerr("Unable to get font size from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }

  gconf_value = gconf_client_get(gconf_client,
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

  gconf_value = gconf_client_get(gconf_client,
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
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  toolbar = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  gconf_value = gconf_client_get(gconf_client,
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

  g_object_unref(G_OBJECT(gconf_client));


  app->action_group = gtk_action_group_new ("terminal-app");
  gtk_action_group_set_translation_domain (app->action_group,
                                           PACKAGE);
  gtk_action_group_add_actions (app->action_group,
                                action_entries,
                                G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (app));
  gtk_action_group_add_toggle_actions (app->action_group,
                                       toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries),
                                       GTK_WIDGET (app));
  gtk_action_group_add_radio_actions (app->action_group,
                                      font_size_action_entries,
                                      G_N_ELEMENTS(font_size_action_entries),
                                      font_size,
                                      G_CALLBACK(terminal_app_action_font_size),
                                      GTK_WIDGET(app));;

  action = gtk_action_group_get_action(app->action_group, "scrollbar");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), scrollbar);

  action = gtk_action_group_get_action(app->action_group, "toolbar");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar);

  action = gtk_action_group_get_action(app->action_group, "reverse");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), reverse);

  app->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (app->ui_manager, app->action_group, 0);
  if (gtk_ui_manager_add_ui_from_string (app->ui_manager,
                                         ui_description, strlen(ui_description),
                                         &error) == 0)
  {
      g_warning ("Unable to create menu: %s", error->message);
      g_error_free (error);
  }

  accel_group = gtk_ui_manager_get_accel_group (app->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (app), accel_group);

#if USE_NOTEBOOK
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER(app), vbox);
  gtk_widget_show (vbox);
#endif

  populate_menubar(app, accel_group);

#if USE_NOTEBOOK
  app->notebook = gtk_notebook_new ();
  g_object_set (G_OBJECT (app->notebook),
                "scrollable", TRUE,
                "show-border", FALSE,
                "tab-hborder", 0,
                "tab-vborder", 0,
                NULL);
  g_signal_connect (G_OBJECT (app->notebook), "notify::page",
                    G_CALLBACK (terminal_app_notify_page), app);
  g_signal_connect (G_OBJECT (app->notebook), "remove",
                    G_CALLBACK (terminal_app_widget_removed), app);
  gtk_box_pack_start (GTK_BOX (vbox), app->notebook, TRUE, TRUE, 0);
  gtk_widget_show (app->notebook);
#endif

  popup = gtk_ui_manager_get_widget (app->ui_manager, "/popup-menu");
  gtk_widget_tap_and_hold_setup(GTK_WIDGET(app), popup, NULL,
                                GTK_TAP_AND_HOLD_NONE);

#ifdef USE_NOTEBOOK
  g_signal_connect (G_OBJECT (app), "destroy",
                    G_CALLBACK (gtk_main_quit), app);
#else
  g_signal_connect (G_OBJECT (app), "destroy",
                    G_CALLBACK (terminal_app_close_window), NULL);
#endif

  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      action = gtk_action_group_get_action (app->action_group, "fullscreen");
      g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
    }

  /* set a unique role on each window (for session management) */
  role = g_strdup_printf ("Terminal-%p-%d-%d", app, getpid (), (gint) time (NULL));
  gtk_window_set_role (GTK_WINDOW (app), role);
  g_free (role);
}



static void
terminal_app_dispose (GObject *object)
{
  parent_class->dispose (object);
}



static void
terminal_app_finalize (GObject *object)
{
  TerminalApp *app = TERMINAL_APP (object);

  g_object_unref (G_OBJECT (app->action_group));
  g_object_unref (G_OBJECT (app->ui_manager));

  parent_class->finalize (object);
}


#ifdef USE_NOTEBOOK
static TerminalWidget*
terminal_app_get_active (TerminalApp *app)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (app->notebook);
  gint         page_num;

  page_num = gtk_notebook_get_current_page (notebook);
  if (G_LIKELY (page_num >= 0))
    return TERMINAL_WIDGET (gtk_notebook_get_nth_page (notebook, page_num));
  else
    return NULL;
}
#endif


#ifdef USE_NOTEBOOK
static void
terminal_app_set_size_force_grid (TerminalApp    *app,
                                  TerminalWidget *widget,
                                  gint            force_grid_width,
                                  gint            force_grid_height)
{
  terminal_app_update_geometry (app);
  terminal_widget_force_resize_window (widget, GTK_WINDOW (app),
                                       force_grid_width, force_grid_height);
}
#endif


#ifdef USE_NOTEBOOK
static void
terminal_app_update_geometry (TerminalApp *app)
{
  TerminalWidget *widget;

  widget = terminal_app_get_active (app);
  if (G_UNLIKELY (widget == NULL))
    return;

  terminal_widget_set_window_geometry_hints (widget, GTK_WINDOW (app));

}
#endif




#ifdef USE_NOTEBOOK
static void
terminal_app_update_actions (TerminalApp *app)
{
  TerminalWidget *terminal;
  GtkNotebook    *notebook = GTK_NOTEBOOK (app->notebook);
  GtkAction      *action;
  gint            page_num;
  gint            n_pages;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    {
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (terminal));
      n_pages = gtk_notebook_get_n_pages (notebook);

      action = gtk_action_group_get_action (app->action_group, "prev-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", page_num > 0,
                    NULL);

      action = gtk_action_group_get_action (app->action_group, "next-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", page_num < n_pages - 1,
                    NULL);

      action = gtk_action_group_get_action (app->action_group, "copy");
      g_object_set (G_OBJECT (action),
                    "sensitive", terminal_widget_has_selection (terminal),
                    NULL);
    }
}
#endif



#ifdef USE_NOTEBOOK
static void
terminal_app_notify_page (GtkNotebook  *notebook,
                          GParamSpec   *pspec,
                          TerminalApp  *app)
{
  TerminalWidget *terminal;
  gchar *terminal_title = NULL;

  terminal = terminal_app_get_active (app);

  terminal_title = terminal_widget_get_title(terminal);
  gtk_window_set_title(GTK_WINDOW(app), terminal_title);
  g_free(terminal_title);

  if (G_LIKELY (terminal != NULL))
    {
      terminal_app_update_actions (app);
    }
}
#endif


#ifdef USE_NOTEBOOK
static void
terminal_app_notify_title(TerminalWidget *terminal,
                          GParamSpec   *pspec,
                          TerminalApp  *app)
{
  TerminalWidget *active_terminal;
  GtkWidget *header;
  gchar *terminal_title = terminal_widget_get_title(terminal);
  active_terminal = terminal_app_get_active (app);
  if (G_LIKELY( terminal == active_terminal )) {
    gtk_window_set_title(GTK_WINDOW(app), terminal_title);
  }

  header = gtk_notebook_get_tab_label(GTK_NOTEBOOK(app->notebook),
		  GTK_WIDGET(terminal));
  g_object_set(header, "title", terminal_title, NULL);
  g_free(terminal_title);
}
#endif

#ifdef USE_NOTEBOOK
static void
terminal_app_context_menu (TerminalWidget  *widget,
                           GdkEvent        *event,
                           TerminalApp     *app)
{
  TerminalWidget *terminal;
  GtkWidget      *popup;
  gint            button = 0;
  gint            time;

  terminal = terminal_app_get_active (app);

  if (G_LIKELY (widget == terminal))
    {
      popup = gtk_ui_manager_get_widget (app->ui_manager, "/popup-menu");
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
		  GtkWidget *item = gtk_ui_manager_get_widget(app->ui_manager,
				  "/popup-menu/open-url");
	      if (msg == NULL)
	        {
		  gtk_widget_hide(item);
		  g_object_set_data(G_OBJECT(app), "url", NULL);
		}
	      else
	        {
		  gtk_widget_show(item);
		  g_object_set_data_full(G_OBJECT(app), "url", msg, g_free);
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
}
#endif



#ifdef USE_NOTEBOOK
static void
terminal_app_widget_removed (GtkNotebook     *notebook,
                             TerminalWidget  *widget,
                             TerminalApp     *app)
{
  TerminalWidget *active;
  gint            npages;

  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (app->notebook));
  if (G_UNLIKELY (npages == 0))
    {
      gtk_widget_destroy (GTK_WIDGET (app));
    }
  else
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (app->notebook), npages > 1);

      active = terminal_app_get_active (app);
      terminal_app_set_size_force_grid (app, active, -1, -1);

      terminal_app_update_actions (app);
    }
}
#endif


static void
terminal_app_open_url (GtkAction	*action,
		       TerminalApp	*app)
{
  osso_context_t *osso = (osso_context_t *)g_object_get_data(
		  G_OBJECT(app), "osso");
  gchar *url = (gchar *)g_object_get_data(G_OBJECT(app), "url");

  if (url && osso) {
    osso_rpc_run_with_defaults(osso,
		    "osso_browser",
		    OSSO_BROWSER_OPEN_NEW_WINDOW_REQ,
		    NULL,
		    DBUS_TYPE_STRING,
		    url,
		    DBUS_TYPE_INVALID);
  }
  g_object_set_data(G_OBJECT(app), "url", NULL);
}


static void
terminal_app_action_new_tab (GtkAction    *action,
                             TerminalApp  *app)
{
  GtkWidget      *terminal;

  terminal = terminal_widget_new ();
#ifdef USE_NOTEBOOK
  TerminalWidget *active;
  const gchar    *directory;

  active = terminal_app_get_active (app);
  if (G_LIKELY (active != NULL))
    {
      directory = terminal_widget_get_working_directory (active);
      terminal_widget_set_working_directory (TERMINAL_WIDGET (terminal),
                                             directory);
    }

#endif
  terminal_app_add (app, TERMINAL_WIDGET (terminal));
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

}

static void
terminal_app_action_close_tab (GtkAction    *action,
                               TerminalApp  *app)
{
  gtk_widget_hide (GTK_WIDGET (app));
  gtk_widget_destroy (GTK_WIDGET (app));

#ifdef USE_NOTEBOOK
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    gtk_widget_destroy (GTK_WIDGET (terminal));
#endif
}


static void
terminal_app_action_copy (GtkAction    *action,
                          TerminalApp  *app)
{
#ifdef USE_NOTEBOOK
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_copy_clipboard (terminal);
#endif
}


static void
terminal_app_action_paste (GtkAction    *action,
                           TerminalApp  *app)
{
#ifdef USE_NOTEBOOK
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
#endif
}

static void
terminal_app_action_edit_shortcuts (GtkAction    *action,
                           TerminalApp  *app)
{
  (void)action;
  (void)app;

  update_shortcut_keys();
}


static void
terminal_app_action_reverse (GtkToggleAction *action,
                             TerminalApp     *app)
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
terminal_app_action_fullscreen (GtkToggleAction *action,
                                TerminalApp     *app)
{
  gboolean fullscreen;

  fullscreen = gtk_toggle_action_get_active (action);
  if (fullscreen) {
      gtk_window_fullscreen(GTK_WINDOW(app));
  } else {
      gtk_window_unfullscreen(GTK_WINDOW(app));
  }
}

static void
terminal_app_action_toolbar (GtkToggleAction *action,
                               TerminalApp     *app)
{
    GConfClient *client;
    gboolean show;

    client = gconf_client_get_default();
    show = gtk_toggle_action_get_active (action);

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_TOOLBAR,
                           show,
                           NULL);

    g_object_unref(G_OBJECT(client));
}

static void
terminal_app_action_scrollbar (GtkToggleAction *action,
                               TerminalApp     *app)
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
terminal_app_action_prev_tab (GtkAction    *action,
                              TerminalApp  *app)
{
#ifdef USE_NOTEBOOK
  gtk_notebook_prev_page (GTK_NOTEBOOK (app->notebook));
#endif
}


static void
terminal_app_action_next_tab (GtkAction    *action,
                              TerminalApp  *app)
{
#ifdef USE_NOTEBOOK
  gtk_notebook_next_page (GTK_NOTEBOOK (app->notebook));
#endif
}

static void
terminal_app_action_font_size (GtkRadioAction *action,
                               GtkRadioAction *current,
                               TerminalApp    *app)
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
terminal_app_action_reset (GtkAction   *action,
                           TerminalApp *app)
{
#ifdef USE_NOTEBOOK
  TerminalWidget *active;

  active = terminal_app_get_active (app);
  terminal_widget_reset (active, FALSE);
#endif
}


static void
terminal_app_action_reset_and_clear (GtkAction    *action,
                                     TerminalApp  *app)
{
#ifdef USE_NOTEBOOK

  TerminalWidget *active;

  active = terminal_app_get_active (app);
  terminal_widget_reset (active, TRUE);
#endif
}

static void
terminal_app_send_ctrl_key(TerminalApp *app,
                           const char *str)
{
  GdkEventKey *key;

  key = (GdkEventKey *) gdk_event_new(GDK_KEY_PRESS);

  key->window = GTK_WIDGET(app)->window;
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
  } else
    gtk_im_context_focus_out(imctx);
  return FALSE;
}

static gboolean
im_context_commit (GtkIMContext *ctx,
                   const gchar *str,
                   ctrl_dialog_data *data)
{
    if (strlen(str) > 0) {
      data->ret = g_strdup(str);
      gtk_dialog_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_ACCEPT);
    }

    return TRUE;
}

static void
terminal_app_action_ctrl (GtkAction    *action,
                          TerminalApp  *app)
{
  ctrl_dialog_data *data;
  GtkWidget *dialog, *label;
  GtkIMContext *imctx;

  dialog = gtk_dialog_new_with_buttons("Control",
                                       GTK_WINDOW(app),
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
    terminal_app_send_ctrl_key(app, data->ret);
    g_free(data->ret);
  }

  g_free(data);
}


static void
terminal_app_action_settings (GtkAction    *action,
                              TerminalApp  *app)
{
    GtkWidget *settings;
    
    settings = terminal_settings_new(GTK_WINDOW(app));
    switch (gtk_dialog_run(GTK_DIALOG(settings))) {
        case GTK_RESPONSE_OK:
            terminal_settings_store(TERMINAL_SETTINGS(settings));
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default:
            break;
    }
    gtk_widget_destroy(settings);
}


static void
terminal_app_action_quit (GtkAction    *action,
                          TerminalApp  *app)
{
  gtk_widget_destroy(GTK_WIDGET(app));
}


/**
 * terminal_app_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_app_new (void)
{
  return g_object_new (TERMINAL_TYPE_APP, NULL);
}


/**
 * terminal_app_add:
 * @app    : A #TerminalApp.
 * @widget : A #TerminalWidget.
 **/
void
terminal_app_add (TerminalApp    *app,
                  TerminalWidget *widget)
{

  gtk_widget_show (GTK_WIDGET (widget));

  if (windows != 0 ) {
    gpointer newapp = NULL;
    newapp = terminal_app_new ();  
    g_object_add_weak_pointer(G_OBJECT(newapp), &newapp);
    gtk_container_add(GTK_CONTAINER (newapp), GTK_WIDGET(widget));
    gtk_widget_show (GTK_WIDGET (newapp));
  } else {
    gtk_container_add(GTK_CONTAINER (app), GTK_WIDGET(widget));
  }

  windows++;

  g_debug ("windows: %d", windows);
#ifdef USE_NOTEBOOK
/*  ExoPropertyProxy *proxy;*/
  TerminalWidget   *active;
  GtkWidget        *header;
  gint              npages;
  gint              page;
  gint              grid_width = -1;
  gint              grid_height = -1;
  gchar            *title;

  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  active = terminal_app_get_active (app);
  if (G_LIKELY (active != NULL))
    {
      terminal_widget_get_size (active, &grid_width, &grid_height);
      terminal_widget_set_size (widget, grid_width, grid_height);
    }

  header = terminal_tab_header_new ();

  title = terminal_widget_get_title(widget);
  g_object_set(header, "title", title, NULL);
  g_free(title);

  g_signal_connect(G_OBJECT(widget), "notify::title",
		  G_CALLBACK(terminal_app_notify_title), app);

  g_signal_connect_swapped (G_OBJECT (header), "close",
                            G_CALLBACK (gtk_widget_destroy), widget);
  gtk_widget_show (header);

  page = gtk_notebook_append_page (GTK_NOTEBOOK (app->notebook),
                                   GTK_WIDGET (widget), header);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (app->notebook),
                                      GTK_WIDGET (widget),
                                      TRUE, TRUE, GTK_PACK_START);
  /* need to show this first, else we cannot switch to it */
  gtk_widget_show (GTK_WIDGET (widget));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (app->notebook), page);
  g_signal_connect (G_OBJECT (widget), "context-menu",
                    G_CALLBACK (terminal_app_context_menu), app);
  g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                            G_CALLBACK (terminal_app_update_actions), app);
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (app->notebook));
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (app->notebook), npages > 1);
  terminal_app_set_size_force_grid (app, widget, grid_width, grid_height);

  terminal_app_update_actions (app);
#endif
}


/**
 * terminal_app_remove:
 * @app     :
 * @widget  :
 **/
void
terminal_app_remove (TerminalApp    *app,
                     TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}

/**
 * terminal_app_launch
 * @app         : A #TerminalApp.
 * @error       : Location to store error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_app_launch (TerminalApp     *app,
		     const gchar     *command,
                     GError          **error)
{
  GtkWidget *terminal;

  g_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();

  terminal_widget_set_working_directory(TERMINAL_WIDGET(terminal),
		 g_get_home_dir()); 

  terminal_app_add (app, TERMINAL_WIDGET (terminal));
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

  /* Keep IM open on startup */
  hildon_gtk_im_context_show(TERMINAL_WIDGET(terminal)->im_context);

  gtk_widget_show(GTK_WIDGET(app));

  return TRUE;
}

#ifndef USE_NOTEBOOK
static void
terminal_app_close_window(GtkWidget *widget, gpointer data)
{
  windows--;
  if (windows == 0) {
    gtk_main_quit ();
  }
}
#endif
