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

#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#define N_(String) String

#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>

#include "terminal-gconf.h"
#include "terminal-settings.h"
#include "terminal-tab-header.h"
#include "terminal-app.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))


static void            terminal_app_dispose                 (GObject         *object);
static void            terminal_app_finalize                (GObject         *object);
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
static void            terminal_app_context_menu            (TerminalWidget  *widget,
                                                             GdkEvent        *event,
                                                             TerminalApp     *app);
static void            terminal_app_widget_removed          (GtkNotebook     *notebook,
                                                             TerminalWidget  *widget,
                                                             TerminalApp     *app);
static void            terminal_app_action_new_tab          (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_close_tab        (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_copy             (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_paste            (GtkAction       *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_reverse          (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_fullscreen       (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_scrollbar        (GtkToggleAction *action,
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

struct _TerminalApp
{
  HildonApp            __parent__;

  GtkActionGroup      *action_group;
  GtkUIManager        *ui_manager;
  
  GtkWidget           *notebook;
};


static GObjectClass *parent_class;

static GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_ ("_File"), },
  { "new-tab", NULL, N_ ("New _Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_new_tab), }, 
  { "close-tab", NULL, N_ ("C_lose Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_close_tab), },
  { "edit-menu", NULL, N_ ("_Edit"),  },
  { "copy", GTK_STOCK_COPY, N_ ("_Copy"), NULL, NULL, G_CALLBACK (terminal_app_action_copy), },
  { "paste", GTK_STOCK_PASTE, N_ ("_Paste"), NULL, NULL, G_CALLBACK (terminal_app_action_paste), },
  { "view-menu", NULL, N_ ("_View"), },
  { "go-menu", NULL, N_ ("_Go"), },
  { "prev-tab", GTK_STOCK_GO_BACK, N_ ("_Previous Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_prev_tab), },
  { "next-tab", GTK_STOCK_GO_FORWARD, N_ ("_Next Tab"), NULL, NULL, G_CALLBACK (terminal_app_action_next_tab), },
  { "font-menu", NULL, N_ ("_Font size"), },
  { "terminal-menu", NULL, N_ ("_Terminal"), },
  { "reset", NULL, N_ ("_Reset"), NULL, NULL, G_CALLBACK (terminal_app_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and C_lear"), NULL, NULL, G_CALLBACK (terminal_app_action_reset_and_clear), },
  { "ctrl", NULL, N_ ("Send Ctrl-<some key>"), NULL, NULL, G_CALLBACK (terminal_app_action_ctrl), },
  { "settings", GTK_STOCK_PREFERENCES, N_ ("_Settings..."), NULL, NULL, G_CALLBACK (terminal_app_action_settings), },
  { "quit", GTK_STOCK_QUIT, N_ ("_Quit"), NULL, NULL, G_CALLBACK (terminal_app_action_quit), },
};

static GtkRadioActionEntry font_size_action_entries[] =
{
  { "10pt", NULL, N_ ("10 pt"), NULL, NULL, 10 },
  { "12pt", NULL, N_ ("12 pt"), NULL, NULL, 12 },
  { "14pt", NULL, N_ ("14 pt"), NULL, NULL, 14 },
  { "16pt", NULL, N_ ("16 pt"), NULL, NULL, 16 },
  { "18pt", NULL, N_ ("18 pt"), NULL, NULL, 18 },
  { "20pt", NULL, N_ ("20 pt"), NULL, NULL, 20 },
  { "22pt", NULL, N_ ("22 pt"), NULL, NULL, 22 },
  { "24pt", NULL, N_ ("24 pt"), NULL, NULL, 24 },
};

static GtkToggleActionEntry toggle_action_entries[] =
{
  { "reverse", NULL, N_ ("_Reverse colors"), NULL, NULL, G_CALLBACK (terminal_app_action_reverse), TRUE },
  { "fullscreen", NULL, N_ ("_Fullscreen"), NULL, NULL, G_CALLBACK (terminal_app_action_fullscreen), FALSE },
  { "scrollbar", NULL, N_ ("_Scrollbar"), NULL, NULL, G_CALLBACK (terminal_app_action_scrollbar), TRUE },
};

static const gchar ui_description[] =
 "<ui>"
 "  <popup name='popup-menu'>"
 "    <menuitem action='new-tab'/>"
 "    <menuitem action='close-tab'/>"
 "    <separator/>"
 "    <menuitem action='copy'/>"
 "    <menuitem action='paste'/>"
 "    <separator/>"
 "    <menuitem action='reverse'/>"
 "    <menuitem action='scrollbar'/>"
 "    <menuitem action='fullscreen'/>"
 "    <separator/>"
 "    <menuitem action='ctrl'/>"
 "    <separator/>"
 "    <menu action='font-menu'>"
 "      <menuitem action='10pt'/>"
 "      <menuitem action='12pt'/>"
 "      <menuitem action='14pt'/>"
 "      <menuitem action='16pt'/>"
 "      <menuitem action='18pt'/>"
 "      <menuitem action='20pt'/>"
 "      <menuitem action='22pt'/>"
 "      <menuitem action='24pt'/>"
 "    </menu>"
 "  </popup>"
 "</ui>";


G_DEFINE_TYPE (TerminalApp, terminal_app, HILDON_TYPE_APP);


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
  HildonAppView       *appview;
  GtkActionGroup      *actiongroup = app->action_group;

  appview = hildon_app_get_appview(HILDON_APP(app));
  menubar = GTK_WIDGET(hildon_appview_get_menu(appview));

  parent = attach_menu(menubar, actiongroup, accelgroup, "file-menu");
  attach_item(parent, actiongroup, accelgroup, "new-tab");
  attach_item(parent, actiongroup, accelgroup, "close-tab");

  parent = attach_menu(menubar, actiongroup, accelgroup, "edit-menu");
  attach_item(parent, actiongroup, accelgroup, "copy");
  attach_item(parent, actiongroup, accelgroup, "paste");

  parent = attach_menu(menubar, actiongroup, accelgroup, "view-menu");
  attach_item(parent, actiongroup, accelgroup, "reverse");
  attach_item(parent, actiongroup, accelgroup, "scrollbar");
  attach_item(parent, actiongroup, accelgroup, "fullscreen");
  gtk_menu_shell_append(GTK_MENU_SHELL(parent),
                        gtk_separator_menu_item_new());
  parent = attach_menu(parent, actiongroup, accelgroup, "font-menu");
  attach_item(parent, actiongroup, accelgroup, "10pt");
  attach_item(parent, actiongroup, accelgroup, "12pt");
  attach_item(parent, actiongroup, accelgroup, "14pt");
  attach_item(parent, actiongroup, accelgroup, "16pt");
  attach_item(parent, actiongroup, accelgroup, "18pt");
  attach_item(parent, actiongroup, accelgroup, "20pt");
  attach_item(parent, actiongroup, accelgroup, "22pt");
  attach_item(parent, actiongroup, accelgroup, "24pt");

  parent = attach_menu(menubar, actiongroup, accelgroup, "go-menu");
  attach_item(parent, actiongroup, accelgroup, "prev-tab");
  attach_item(parent, actiongroup, accelgroup, "next-tab");

  parent = attach_menu(menubar, actiongroup, accelgroup, "terminal-menu");
  attach_item(parent, actiongroup, accelgroup, "reset");
  attach_item(parent, actiongroup, accelgroup, "reset-and-clear");
  attach_item(parent, actiongroup, accelgroup, "ctrl");

  /*attach_item(menubar, actiongroup, accelgroup, "settings");*/

  attach_item(menubar, actiongroup, accelgroup, "quit");

  gtk_widget_show_all(menubar);
}

static int
terminal_app_get_font_size(TerminalApp *app) {
    GtkAction *action;

    action = gtk_action_group_get_action(app->action_group, "10pt");
    if (!action) {
        return -1;
    }

    return gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
}

static
gboolean terminal_app_set_font_size(TerminalApp *app, int new_size) {
    GtkAction *action;
    char new_name[5];

    if (new_size < 10 || new_size > 24) {
        return FALSE;
    }

    snprintf(new_name, sizeof(new_name), "%dpt", new_size);

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
    HildonAppView *appview;
    int font_size;
    GtkAction *action;

    switch (event->keyval) 
    {
        case GDK_F6: /* Full screen */
            appview = hildon_app_get_appview(HILDON_APP(app));
            fs = hildon_appview_get_fullscreen(appview);
            action = gtk_action_group_get_action(app->action_group,
                                                 "fullscreen");
            hildon_appview_set_fullscreen(appview, !fs);
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
                                         !fs);
            return TRUE;

        case GDK_F7: /* Zoom in */
            font_size = terminal_app_get_font_size(app);
            if (font_size < 0) {
                gtk_infoprint(GTK_WINDOW(app), "Getting font size failed!");
                return TRUE;
            }

            if (font_size >= 24) {
                gtk_infoprint(GTK_WINDOW(app), "Already at maximum font size");
                return TRUE;
            }
            terminal_app_set_font_size(app, font_size + 2);
            return TRUE;

        case GDK_F8: /* Zoom out */
            font_size = terminal_app_get_font_size(app);
            if (font_size < 0) {
                gtk_infoprint(GTK_WINDOW(app), "Getting font size failed!");
                return TRUE;
            }
            
            if (font_size <= 10) {
                gtk_infoprint(GTK_WINDOW(app), "Already at minimum font size");
                return TRUE;
            }
            terminal_app_set_font_size(app, font_size - 2);
            return TRUE;
    }

    return FALSE;
}

static void
terminal_app_init (TerminalApp *app)
{
  HildonAppView       *appview;
  GtkAction           *action;
  GtkAccelGroup       *accel_group;
  GtkWidget           *vbox, *popup;
  GError              *error = NULL;
  gchar               *role;
  gint                 font_size;
  gboolean             scrollbar, reverse;
  GConfClient         *gconf_client;
  GConfValue          *gconf_value;


  hildon_app_set_title(HILDON_APP(app), _("X Terminal"));
  hildon_app_set_two_part_title(HILDON_APP(app), FALSE);
  appview = HILDON_APPVIEW( hildon_appview_new("osso_xterm") );
  hildon_app_set_appview(HILDON_APP(app), appview);
  g_signal_connect( G_OBJECT(appview), "destroy",
              G_CALLBACK(gtk_main_quit), NULL );

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

  if (font_size == 0) {
      font_size = OSSO_XTERM_DEFAULT_FONT_SIZE;
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

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER(appview), vbox);
  gtk_widget_show (vbox);

  populate_menubar(app, accel_group);

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

  popup = gtk_ui_manager_get_widget (app->ui_manager, "/popup-menu");
  gtk_widget_tap_and_hold_setup(GTK_WIDGET(appview), popup, NULL,
                                GTK_TAP_AND_HOLD_NONE);

  g_signal_connect (G_OBJECT (app), "delete-event",
                    G_CALLBACK (gtk_widget_destroy), app);

  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      hildon_appview_set_fullscreen_key_allowed(appview, TRUE);
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



static void
terminal_app_update_geometry (TerminalApp *app)
{
  TerminalWidget *widget;

  widget = terminal_app_get_active (app);
  if (G_UNLIKELY (widget == NULL))
    return;

  terminal_widget_set_window_geometry_hints (widget, GTK_WINDOW (app));
}



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



static void
terminal_app_notify_page (GtkNotebook  *notebook,
                          GParamSpec   *pspec,
                          TerminalApp  *app)
{
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    {
      terminal_app_update_actions (app);
    }
}



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
      if (G_UNLIKELY (popup == NULL))
        return;

      if (event != NULL)
        {
          if (event->type == GDK_BUTTON_PRESS)
            button = event->button.button;
          time = event->button.time;
        }
      else
        {
          time = gtk_get_current_event_time ();
        }

      gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                      NULL, NULL, button, time);
    }
}



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



static void
terminal_app_action_new_tab (GtkAction    *action,
                             TerminalApp  *app)
{
  TerminalWidget *active;
  const gchar    *directory;
  GtkWidget      *terminal;

  terminal = terminal_widget_new ();

  active = terminal_app_get_active (app);
  if (G_LIKELY (active != NULL))
    {
      directory = terminal_widget_get_working_directory (active);
      terminal_widget_set_working_directory (TERMINAL_WIDGET (terminal),
                                             directory);
    }

  terminal_app_add (app, TERMINAL_WIDGET (terminal));
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));
}


static void
terminal_app_action_close_tab (GtkAction    *action,
                               TerminalApp  *app)
{
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    gtk_widget_destroy (GTK_WIDGET (terminal));
}


static void
terminal_app_action_copy (GtkAction    *action,
                          TerminalApp  *app)
{
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_copy_clipboard (terminal);
}


static void
terminal_app_action_paste (GtkAction    *action,
                           TerminalApp  *app)
{
  TerminalWidget *terminal;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
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
  HildonAppView *appview;
  gboolean fullscreen;

  appview = hildon_app_get_appview(HILDON_APP(app));
  fullscreen = gtk_toggle_action_get_active (action);
  hildon_appview_set_fullscreen (appview, fullscreen);
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
  gtk_notebook_prev_page (GTK_NOTEBOOK (app->notebook));
}


static void
terminal_app_action_next_tab (GtkAction    *action,
                              TerminalApp  *app)
{
  gtk_notebook_next_page (GTK_NOTEBOOK (app->notebook));
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
  TerminalWidget *active;

  active = terminal_app_get_active (app);
  terminal_widget_reset (active, FALSE);
}


static void
terminal_app_action_reset_and_clear (GtkAction    *action,
                                     TerminalApp  *app)
{
  TerminalWidget *active;

  active = terminal_app_get_active (app);
  terminal_widget_reset (active, TRUE);
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
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
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
/*  ExoPropertyProxy *proxy;*/
  TerminalWidget   *active;
  GtkWidget        *header, *popup;
  gint              npages;
  gint              page;
  gint              grid_width = -1;
  gint              grid_height = -1;

  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  popup = gtk_ui_manager_get_widget (app->ui_manager, "/popup-menu");
  gtk_widget_tap_and_hold_setup(GTK_WIDGET(widget->terminal), popup, NULL,
                                GTK_TAP_AND_HOLD_NONE);

  active = terminal_app_get_active (app);
  if (G_LIKELY (active != NULL))
    {
      terminal_widget_get_size (active, &grid_width, &grid_height);
      terminal_widget_set_size (widget, grid_width, grid_height);
    }

  header = terminal_tab_header_new ();
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
                     GError          **error)
{
  GtkWidget *terminal;

  g_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();

  terminal_app_add (app, TERMINAL_WIDGET (terminal));
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

  /* Keep IM open on startup */
  hildon_gtk_im_context_show(TERMINAL_WIDGET(terminal)->im_context);

  gtk_widget_show(GTK_WIDGET(app));

  return TRUE;
}

