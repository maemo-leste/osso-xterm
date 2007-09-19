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
#include "terminal-app.h"
#include "terminal-app-wrapper.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

static void            terminal_app_wrapper_dispose                 (GObject         *object);
static void            terminal_app_wrapper_finalize                (GObject         *object);
#if 0
static void            terminal_app_wrapper_update_actions          (TerminalAppWrapper     *app);
static void            terminal_app_wrapper_context_menu            (TerminalWidget  *widget,
                                                             GdkEvent        *event,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_notify_title            (TerminalWidget  *terminal,
                                                             GParamSpec      *pspec,
                                                               TerminalAppWrapper     *app);
static void            terminal_app_wrapper_open_url                (GtkAction       *action,
		                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_new_window          (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
#endif
static void            terminal_app_wrapper_action_copy             (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_paste            (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_reverse          (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_fullscreen       (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_scrollbar        (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_toolbar        (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
#if 0
static void            terminal_app_wrapper_action_font_size        (GtkRadioAction  *action,
                                                             GtkRadioAction  *current,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_reset            (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_reset_and_clear  (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_ctrl             (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
#endif
static void            terminal_app_wrapper_action_settings         (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_quit             (GtkAction       *action,
                                                             TerminalAppWrapper     *app);

static void            terminal_app_wrapper_close_window            (GtkAction *action, 
                                                             TerminalAppWrapper     *app);

static void            terminal_app_wrapper_action_show_full_screen (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_show_normal_screen(GtkToggleAction *action,
                                                              TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_new_window(GtkToggleAction *action,
                                                              TerminalAppWrapper     *app);
static void            terminal_app_wrapper_set_toolbar (gboolean show);
static void            terminal_app_wrapper_select_all         (GtkAction       *action,
                                                             TerminalAppWrapper     *app);

static gboolean        terminal_app_key_press_event (TerminalAppWrapper *app, 
						     GdkEventKey *event,
						     gpointer user_data);

static GObjectClass *parent_class;

static GtkActionEntry action_entries[] =
{

  { "font-menu", NULL, N_ ("Font size"), },
  { "view-menu", NULL, ("webb_me_view"), },
  { "edit-menu", NULL, N_("weba_me_edit"),  },
  { "copy", NULL, N_("weba_me_copy"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_copy), },
  { "paste", NULL, N_("weba_me_paste"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_paste), },
  { "tools-menu", NULL, N_("weba_me_tools"), },
  { "close-menu", NULL, N_("weba_me_close"), },
  { "windows-menu", NULL, ("weba_me_windows"), },
  { "new-window", NULL, N_("weba_me_new_window"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_new_window), }, 
  { "tools", NULL, N_("weba_me_tools"), },
  { "close-window", NULL, N_("weba_me_close_window"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_close_window), },
  { "close-all-windows", NULL, N_("weba_me_close_all_windows"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_quit), },

  { "show-toolbar-menu", NULL, N_("webb_me_show_toolbar"), },

  { "settings", NULL, N_("weba_me_settings"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_settings), },
  { "select-all", NULL, ("weba_me_select_all"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_select_all), },

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
  { "reverse", NULL, N_ ("Reverse colors"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_reverse), TRUE },
  { "scrollbar", NULL, N_ ("Scrollbar"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_scrollbar), TRUE },
  { "toolbar", NULL, N_ ("Toolbar"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_toolbar), TRUE },

  { "fullscreen", NULL, N_ ("webb_me_full_screen"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_fullscreen), FALSE },
  { "show-full-screen", NULL, N_ ("webb_me_full_screen"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_show_full_screen), TRUE},
  { "show-normal-screen", NULL, N_ ("webb_me_normal_mode"), NULL, NULL, G_CALLBACK(terminal_app_wrapper_action_show_normal_screen), TRUE},
};

static const gchar ui_description[] =
 "<ui>"
 "  <popup name='popup-menu'>"
 "    <menuitem action='new-window'/>"
 "    <menuitem action='close-window'/>"
 "    <separator/>"
 "    <menuitem action='copy'/>"
 "    <menuitem action='paste'/>"
 "    <separator/>"
 "    <menuitem action='settings'/>"
 "  </popup>"
 "</ui>";

static gboolean toolbar_fs = TRUE;
static gboolean toolbar_normal = TRUE;
static gboolean fs = FALSE;

static gint window_id = 1;

G_DEFINE_TYPE (TerminalAppWrapper, terminal_app_wrapper, HILDON_TYPE_WINDOW);

typedef struct {
    GtkWidget *dialog;
    gchar *ret;
} ctrl_dialog_data;


static void
terminal_app_wrapper_class_init (TerminalAppWrapperClass *klass)
{
  GObjectClass *gobject_class;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_app_wrapper_dispose;
  gobject_class->finalize = terminal_app_wrapper_finalize;
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

#if 0
static void
on_window_menu_select (GtkWidget *widget, TerminalAppWrapper *app)
{
    g_assert (TERMINAL_IS_APP (app));
    g_debug (__FUNCTION__);
    gtk_window_present ( GTK_WINDOW (app));
}

static void
remove_item (gpointer data, GObject *obj)
{
    g_debug (__FUNCTION__);
    if (obj == NULL) {
        g_debug ("There must be error");
    }
    if (data != NULL && GTK_IS_WIDGET (data)) {
        gtk_widget_destroy (GTK_WIDGET (data));
    }
}

#endif

struct appstru
{
  TerminalAppWrapper *appw;
  TerminalApp *app;
};

static void
on_menu_item_activated (GtkWidget *menuitem, gpointer data) 
{
  struct appstru *apps = (struct appstru *)data;

  TerminalAppWrapper *appw = TERMINAL_APP_WRAPPER (apps->appw);
  TerminalApp *app = TERMINAL_APP (apps->app);
  if (app != appw->current) {
    /* FIXME: implement */
  }
}

static void
on_menu_item_destroy (GtkWidget *menuitem, gpointer data) 
{
  struct appstru *apps = (struct appstru *)data;
  g_free (apps);
}

static void
attach_new_window (TerminalAppWrapper *appw, TerminalApp *app, const gchar *label)
{
    GtkWidget *menuitem;
    GtkRadioAction *action = gtk_radio_action_new (label, 
						   label, 
						   NULL, 
						   label, 1);

    gtk_action_group_add_action (appw->action_group, GTK_ACTION (action));

    gtk_radio_action_set_group (action, appw->window_group);
    appw->window_group = gtk_radio_action_get_group (action);

    menuitem = gtk_action_create_menu_item(GTK_ACTION (action));
    gtk_menu_shell_prepend(GTK_MENU_SHELL(appw->windows_menu), menuitem);
    appw->apps = g_slist_append (appw->apps, app);
    
    struct appstru *apps = g_new0 (struct appstru, 1);
    apps->appw = appw;
    apps->app = app;
    g_object_set (menuitem, "active", TRUE, NULL);
    g_signal_connect (menuitem, "activate", G_CALLBACK (on_menu_item_activated), apps);
    g_signal_connect (menuitem, "destroy", G_CALLBACK (on_menu_item_destroy), apps);
}


static void
populate_menubar (TerminalAppWrapper *app, GtkAccelGroup *accelgroup)
{
  GtkActionGroup      *actiongroup = app->action_group;
  GtkWidget           *parent = NULL, *subparent;
  GtkWidget *menubar = NULL;

  menubar = gtk_menu_new();

  app->windows_menu = attach_menu(menubar, actiongroup, accelgroup, "windows-menu");

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

  parent = attach_menu(menubar, actiongroup, accelgroup, "close-menu");
  attach_item(parent, actiongroup, accelgroup, "close-window");
  attach_item(parent, actiongroup, accelgroup, "close-all-windows");
  
  hildon_window_set_menu(HILDON_WINDOW(app), GTK_MENU(menubar));

  GtkWidget *menuitem = gtk_separator_menu_item_new();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(app->windows_menu), 
			menuitem);
  attach_item(app->windows_menu, actiongroup, accelgroup, "new-window");
  gtk_widget_show_all(menubar);

}

#if 0
static int
terminal_app_wrapper_get_font_size(TerminalAppWrapper *app) {

  //    return terminal_get_font_size(app->current);

}

static
gboolean terminal_app_wrapper_set_font_size(TerminalAppWrapper *app, int new_size) {

  //  return terminal_app_wrapper_set_font_size(app, new_size);
}
#endif

static gboolean
terminal_app_key_press_event (TerminalAppWrapper *app,
                              GdkEventKey *event,
                              gpointer user_data) 
{

    int font_size;
    GtkAction *action;

    switch (event->keyval) 
    {
        case HILDON_HARDKEY_FULLSCREEN: /* Full screen */
	  action = gtk_action_group_get_action( app->action_group,
                                                 "fullscreen");
            fs = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	    if (fs) {
	      gtk_window_unfullscreen(GTK_WINDOW(app));
	      //terminal_app_set_toolbar (app->current, toolbar_normal);
	    } else {
 		    gtk_window_fullscreen(GTK_WINDOW(app));
		    //terminal_app_set_toolbar (app->current, toolbar_fs);
	    }
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
                                         !fs);
            return TRUE;

        case HILDON_HARDKEY_INCREASE: /* Zoom in */
            font_size = terminal_app_get_font_size(app->current);
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
            terminal_app_set_font_size(app->current, font_size + 2);
            return TRUE;

        case HILDON_HARDKEY_DECREASE: /* Zoom out */
            font_size = terminal_app_get_font_size(app->current);
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
            terminal_app_set_font_size(app->current, font_size - 2);
            return TRUE;
	case HILDON_HARDKEY_HOME: /* Ignoring... */
	    return TRUE;
    }

    return FALSE;
}


static void
terminal_app_wrapper_init (TerminalAppWrapper *app)
{
  GtkAction           *action;
  GtkAccelGroup       *accel_group;
  GtkWidget           *popup;
  GError              *error = NULL;
  gchar               *role;
  gint                 font_size = 0;
  gboolean             scrollbar = FALSE, toolbar = FALSE, reverse = FALSE;
  GConfClient         *gconf_client;
  GConfValue          *gconf_value;
  HildonProgram       *program;

  program = hildon_program_get_instance();
  hildon_program_add_window(program, HILDON_WINDOW(app));

  app->action_group = gtk_action_group_new ("terminal-app");
  gtk_action_group_set_translation_domain (app->action_group,
                                           GETTEXT_PACKAGE);
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

  action = gtk_action_group_get_action(app->action_group, "show-full-screen");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar_fs);
  action = gtk_action_group_get_action(app->action_group, "show-normal-screen");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar_normal);

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

  populate_menubar(app, accel_group);

  popup = gtk_ui_manager_get_widget (app->ui_manager, "/popup-menu");
  gtk_widget_tap_and_hold_setup(GTK_WIDGET(app), popup, NULL,
                                GTK_TAP_AND_HOLD_NONE);

  gtk_window_set_title(GTK_WINDOW(app), "XTerm");

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

  g_debug ("End of the init");
}


static void
terminal_app_wrapper_dispose (GObject *object)
{
  g_debug (__FUNCTION__);
  parent_class->dispose (object);
}

static void
terminal_app_wrapper_finalize (GObject *object)
{
  TerminalAppWrapper *app = TERMINAL_APP_WRAPPER (object);

  g_debug (__FUNCTION__);

  for (GSList *list = g_slist_nth (app->apps, 0); list != NULL; 
       list = g_slist_next (list)) {
    g_object_unref (list->data);
  }

  parent_class->finalize (object);
}

#if 0
static void
terminal_app_wrapper_open_url (GtkAction	*action,
		       TerminalAppWrapper	*app)
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
#endif

static void
terminal_app_wrapper_action_copy (GtkAction    *action,
                          TerminalAppWrapper  *app)
{

}


static void
terminal_app_wrapper_action_paste (GtkAction    *action,
                           TerminalAppWrapper  *app)
{
}

static void
terminal_app_wrapper_action_reverse (GtkToggleAction *action,
                             TerminalAppWrapper     *app)
{
  //  terminal_app_action_reverse (action, app->current);

}


static void
terminal_app_wrapper_action_fullscreen (GtkToggleAction *action,
                                TerminalAppWrapper     *app)
{
  gboolean fullscreen;

  fullscreen = gtk_toggle_action_get_active (action);
  if (fullscreen) {
      gtk_window_fullscreen(GTK_WINDOW(app));
      terminal_app_wrapper_set_toolbar (toolbar_fs);
      fs = TRUE;
  } else {
      gtk_window_unfullscreen(GTK_WINDOW(app));
      terminal_app_wrapper_set_toolbar (toolbar_normal);
      fs = FALSE;
  }
}

static void
terminal_app_wrapper_set_toolbar (gboolean show)
{
  //  terminal_app_set_toolbar (show);
}

static void
terminal_app_wrapper_action_toolbar (GtkToggleAction *action,
                               TerminalAppWrapper     *app)
{
    gboolean show;

    show = gtk_toggle_action_get_active (action);
    //    terminal_app_set_toolbar (show);
    

}

static void
terminal_app_wrapper_action_scrollbar (GtkToggleAction *action,
                               TerminalAppWrapper     *app)
{

}

#if 0
static void
terminal_app_wrapper_action_prev_tab (GtkAction    *action,
                              TerminalAppWrapper  *app)
{

}


static void
terminal_app_wrapper_action_next_tab (GtkAction    *action,
                              TerminalAppWrapper  *app)
{

}

static void
terminal_app_wrapper_action_font_size (GtkRadioAction *action,
                               GtkRadioAction *current,
                               TerminalAppWrapper    *app)
{
}


static void
terminal_app_wrapper_action_reset (GtkAction   *action,
                           TerminalAppWrapper *app)
{

}


static void
terminal_app_wrapper_action_reset_and_clear (GtkAction    *action,
                                     TerminalAppWrapper  *app)
{

}


static void
terminal_app_wrapper_send_ctrl_key(TerminalAppWrapper *app,
				   const char *str)
{
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
terminal_app_wrapper_action_ctrl (GtkAction    *action,
                          TerminalAppWrapper  *app)
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
    terminal_app_wrapper_send_ctrl_key(app, data->ret);
    g_free(data->ret);
  }

  g_free(data);
}
#endif

static void
terminal_app_wrapper_action_settings (GtkAction    *action,
                              TerminalAppWrapper  *app)
{

}


static void
terminal_app_wrapper_action_quit (GtkAction    *action,
                          TerminalAppWrapper  *app)
{
  gtk_main_quit ();
}


/**
 * terminal_app_wrapper_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_app_wrapper_new (void)
{
  return g_object_new (TERMINAL_TYPE_APP_WRAPPER, NULL);
}



gboolean
terminal_app_wrapper_launch (TerminalAppWrapper     *appw,
		     const gchar     *command,
                     GError          **error)
{
  g_return_val_if_fail (TERMINAL_IS_APP_WRAPPER (appw), FALSE);

  g_debug (__FUNCTION__);

  gpointer app = terminal_app_new ();
  g_object_add_weak_pointer(G_OBJECT(app), &app);
  if (!terminal_app_launch (TERMINAL_APP(app), command, error)) {
      return FALSE;
  }

  appw->current = app;

  gtk_widget_show (GTK_WIDGET (app));
  gtk_container_add (GTK_CONTAINER (appw), GTK_WIDGET (app));

  gtk_widget_show(GTK_WIDGET(appw));

  attach_new_window (appw, app, N_("XTerm"));

  appw->windows = 0;

  return TRUE;
}

static void
remove_old_and_replace_with_new (TerminalApp *app, TerminalApp *app_new)
{
  g_debug (__FUNCTION__);

  GtkWidget *pwindow = GTK_WIDGET (gtk_widget_get_parent (GTK_WIDGET (app)));
  GtkWidget *pwindow_new = GTK_WIDGET (gtk_widget_get_parent (GTK_WIDGET (app_new)));

  gtk_container_remove (GTK_CONTAINER (pwindow), GTK_WIDGET (app));

  g_object_ref (app_new);
  gtk_container_remove (GTK_CONTAINER (pwindow_new), GTK_WIDGET (app_new));
  gtk_container_add (GTK_CONTAINER (pwindow), GTK_WIDGET (app_new));
  g_object_unref (app_new);

  g_object_unref (pwindow_new);
}

static void
terminal_app_wrapper_close_window(GtkAction *action, TerminalAppWrapper *appw)
{
    g_assert (appw);
    g_assert (TERMINAL_IS_APP_WRAPPER (appw));

    GSList *list = NULL;

    g_debug (__FUNCTION__);

    if (appw->apps == NULL) {
      g_debug ("appw->apps == NULL");
      gtk_main_quit ();
      return;
    }

    list = g_slist_nth(appw->apps, 0);

    if (list != NULL && list->data != NULL && appw->current != (list->data)) {
      g_debug ("first");
      remove_old_and_replace_with_new (appw->current, (list->data));
      appw->current = (list->data);
      appw->windows--;
      return;
    }
    list = g_slist_nth(appw->apps, 1);
    if (list != NULL && list->data != NULL && appw->current != (list->data)) {
      g_debug ("second");
      remove_old_and_replace_with_new (appw->current, (list->data));
      appw->current = (list->data);
      appw->windows--;
    } else {
      gtk_main_quit ();
    }
}

static void            
terminal_app_wrapper_action_show_full_screen (GtkToggleAction *action,
                                      TerminalAppWrapper     *app)
{
    toolbar_fs = gtk_toggle_action_get_active (action);
    if (fs == TRUE) {
        terminal_app_wrapper_set_toolbar (toolbar_fs);  
    }
}

static void
terminal_app_wrapper_action_show_normal_screen(GtkToggleAction *action,
                                       TerminalAppWrapper     *app)
{
    toolbar_normal = gtk_toggle_action_get_active (action);
    if (fs == FALSE) {
        terminal_app_wrapper_set_toolbar (toolbar_normal);      
    }
}

static void
terminal_app_wrapper_select_all (GtkAction    *action,
                              TerminalAppWrapper  *app)
{
    g_assert (app != NULL);
    g_assert (TERMINAL_IS_APP (app));
    /* terminal_widget_select_all (TERMINAL_WIDGET (app->terminal)); */
    g_debug(__FUNCTION__);
}

static void            
terminal_app_wrapper_action_new_window (GtkToggleAction *action,
                                        TerminalAppWrapper     *appw)
{

  GtkWidget *fake = hildon_window_new ();
  gpointer app = terminal_app_new ();
  gchar window_name[256];

  g_snprintf (window_name, 255, "XTerm (%d)", window_id++);

  g_object_add_weak_pointer(G_OBJECT(app), &app);
  terminal_app_new_window (app);

  //  g_signal_connect (fake, "focus", on_focus, )

  g_object_ref (appw->current);
  gtk_container_remove (GTK_CONTAINER (appw), GTK_WIDGET (appw->current));

  gtk_container_add (GTK_CONTAINER (fake), GTK_WIDGET (appw->current));
  gtk_window_set_title(GTK_WINDOW(fake), window_name);
  gtk_widget_show (fake);
  g_object_unref (appw->current);

  appw->current = app;
  gtk_widget_show (GTK_WIDGET (app));
  gtk_container_add (GTK_CONTAINER (appw), GTK_WIDGET (app));

  attach_new_window (appw, app, window_name);
  appw->windows++;

}
