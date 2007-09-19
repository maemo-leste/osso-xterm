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
#include "terminal-settings.h"
#include "terminal-tab-header.h"
#include "terminal-app.h"
#include "shortcuts.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

static void            terminal_app_dispose                 (GObject         *object);
static void            terminal_app_finalize                (GObject         *object);
static void            terminal_app_update_actions          (TerminalApp     *app);
static void            terminal_app_context_menu            (TerminalWidget  *widget,
                                                             GdkEvent        *event,
                                                             TerminalApp     *app);
static void            terminal_app_notify_title            (TerminalWidget  *terminal,
                                                             GParamSpec      *pspec,
                                                               TerminalApp     *app);
#if 0
static void            terminal_app_open_url                (GtkAction       *action,
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

static void            terminal_app_close_window            (GtkAction *action, 
                                                             TerminalApp     *app);
static void            terminal_app_action_show_full_screen (GtkToggleAction *action,
                                                             TerminalApp     *app);
static void            terminal_app_action_show_normal_screen(GtkToggleAction *action,
                                                              TerminalApp     *app);

static void            terminal_app_select_all         (GtkAction       *action,
                                                             TerminalApp     *app);
/* This keeps count of number of windows */
#endif

/* Show toolbar */
//static gboolean toolbar_fs = TRUE;
//static gboolean toolbar_normal = TRUE;
//static gboolean fs = FALSE;

static GObjectClass *parent_class;

G_DEFINE_TYPE (TerminalApp, terminal_app, GTK_TYPE_VBOX);


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

void
terminal_app_new_window (TerminalApp  *app)
{
  GtkWidget      *terminal;

  terminal = terminal_widget_new ();
  const gchar    *directory;

  g_assert (TERMINAL_IS_WIDGET (terminal));

  directory = terminal_widget_get_working_directory (TERMINAL_WIDGET (terminal));
  if (directory == NULL) {
    directory = g_get_home_dir(); 
    terminal_widget_set_working_directory (TERMINAL_WIDGET (terminal),
                                           directory);
  }
  terminal_app_add (app, TERMINAL_WIDGET (terminal));
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

}

#if 0
static void
populate_menubar (TerminalApp *app, GtkAccelGroup *accelgroup)
{
  GtkActionGroup      *actiongroup = app->action_group;
  GtkWidget           *parent = NULL, *subparent;
  GtkWidget *menubar = NULL;

    menubar = gtk_menu_new();

    //if (windows == 0) {
    app->windows_menu = attach_menu(menubar, actiongroup, accelgroup, "windows-menu");
    //}
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


  for (GSList *list = g_slist_nth (window_list, 0); list != NULL; 
	 list = g_slist_next (list) ) {
    GtkWidget *menuitem = gtk_action_create_menu_item(
				       GTK_ACTION (TERMINAL_APP(list->data)->menuaction));
    gtk_menu_shell_prepend(GTK_MENU_SHELL(app->windows_menu), 
			   menuitem);

    gtk_radio_action_set_group (GTK_RADIO_ACTION (TERMINAL_APP(list->data)->menuaction), app_window_group);
    app_window_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (TERMINAL_APP(list->data)->menuaction));

    g_object_weak_ref (G_OBJECT (TERMINAL_APP (list->data)->menuaction), remove_item, menuitem);
    g_object_unref (TERMINAL_APP(list->data)->menuaction);

  }

  gchar window_name[256];
  if(windows != 0)
    g_snprintf (window_name, 255, "XTerm (%d)", window_id++);
  else
    g_snprintf (window_name, 7, "XTerm");

  /* This for current app */
  attach_new_window (app, app, window_name);

  /* This for previous apps */
  for (GSList *list = g_slist_nth (window_list, 0); list != NULL; 
	 list = g_slist_next (list) ) {
    GtkWidget *menuitem = gtk_action_create_menu_item(GTK_ACTION (app->menuaction));
    gtk_menu_shell_prepend(GTK_MENU_SHELL(TERMINAL_APP (list->data)->windows_menu), 
			   menuitem);
    g_object_weak_ref (G_OBJECT (app->menuaction), remove_item, menuitem);
    g_object_unref (app->menuaction);

    gtk_radio_action_set_group (GTK_RADIO_ACTION (app->menuaction), app_window_group);
    app_window_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (app->menuaction));
  }

  GtkWidget *menuitem = gtk_separator_menu_item_new();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(app->windows_menu), 
			menuitem);
  attach_item(app->windows_menu, actiongroup, accelgroup, "new-window");
  app->menubar = menubar;
  gtk_widget_show_all(app->menubar);
}
#endif


int
terminal_app_get_font_size(TerminalApp *app) {
    GtkAction *action;

    action = gtk_action_group_get_action(app->action_group, "-8pt");
    if (!action) {
        return 0xf00b4;
    }

    return gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
}

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

#if 0
static gboolean
terminal_app_key_press_event (TerminalApp *app,
                              GdkEventKey *event,
                              gpointer user_data) {

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
            terminal_app_set_toolbar (toolbar_normal);
	    } else {
 		    gtk_window_fullscreen(GTK_WINDOW(app));
            terminal_app_set_toolbar (toolbar_fs);
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
#endif

static void
terminal_app_init (TerminalApp *app)
{
  app->terminal = NULL;
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
//  GtkWidget *menuitem = NULL;

  g_debug (__FUNCTION__);

  g_object_unref (app->terminal);
  //g_object_unref (G_OBJECT (app->action_group));
  //g_object_unref (G_OBJECT (app->ui_manager));

  parent_class->finalize (object);
}


static TerminalWidget*
terminal_app_get_active (TerminalApp *app)
{
  return TERMINAL_WIDGET (app->terminal);    
}

static void
terminal_app_update_actions (TerminalApp *app)
{
  g_debug (__FUNCTION__);
#if 0
  TerminalWidget *terminal;
  GtkAction      *action;

  terminal = terminal_app_get_active (app);
  if (G_LIKELY (terminal != NULL))
    {
      action = gtk_action_group_get_action (app->action_group, "copy");
      g_object_set (G_OBJECT (action),
                    "sensitive", terminal_widget_has_selection (terminal),
                    NULL);
    }
#endif
}


static void
terminal_app_notify_title(TerminalWidget *terminal,
                          GParamSpec   *pspec,
                          TerminalApp  *app)
{
  TerminalWidget *active_terminal;
  gchar *terminal_title = terminal_widget_get_title(terminal);
  active_terminal = terminal_app_get_active (app);
  if (G_LIKELY( terminal == active_terminal )) {
    gtk_window_set_title(GTK_WINDOW(app), terminal_title);
  }
  g_free(terminal_title);
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


#if 0
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
terminal_app_action_close_tab (GtkAction    *action,
                               TerminalApp  *app)
{
  gtk_widget_hide (GTK_WIDGET (app));
  gtk_widget_destroy (GTK_WIDGET (app));

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
      terminal_app_set_toolbar (toolbar_fs);
      fs = TRUE;
  } else {
      gtk_window_unfullscreen(GTK_WINDOW(app));
      terminal_app_set_toolbar (toolbar_normal);
      fs = FALSE;
  }
}

void
terminal_app_set_toolbar (TerminalApp *app, gboolean show)
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
terminal_app_action_toolbar (GtkToggleAction *action,
                               TerminalApp     *app)
{
    gboolean show;

    show = gtk_toggle_action_get_active (action);
    terminal_app_set_toolbar (show);

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
}


static void
terminal_app_action_next_tab (GtkAction    *action,
                              TerminalApp  *app)
{
}
#endif

void
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

#if 0
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
  //  gtk_widget_destroy(GTK_WIDGET(app));
  gtk_main_quit ();
}
#endif


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


static void
terminal_app_real_add (TerminalApp    *app,
		       TerminalWidget *widget)
{
  /*
    gchar *title = terminal_widget_get_title(widget);
    g_object_set(GTK_WINDOW (app), "title", title, NULL);
    g_free(title);
  */
    gtk_container_add(GTK_CONTAINER (app), GTK_WIDGET(widget));
    TERMINAL_APP (app)->terminal = widget;

    g_signal_connect(G_OBJECT(widget), "notify::title",
  		    G_CALLBACK(terminal_app_notify_title), app);
    g_signal_connect (G_OBJECT (widget), "context-menu",
                      G_CALLBACK (terminal_app_context_menu), app);
    g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                              G_CALLBACK (terminal_app_update_actions), app);
    terminal_app_update_actions (app);

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
  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_show (GTK_WIDGET (widget));

  terminal_app_real_add (app, widget);

  g_object_ref (widget);

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

  g_debug (__FUNCTION__);

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

  //  gtk_widget_show(GTK_WIDGET(app));

  return TRUE;
}

#if 0
static void
terminal_app_close_window(GtkAction *action, TerminalApp *app)
{
    g_assert (app);
    g_assert (TERMINAL_IS_APP (app));

    g_debug (__FUNCTION__);

    //gtk_widget_hide (GTK_WIDGET (app));
    //gtk_widget_destroy (GTK_WIDGET (app));
}

static void            
terminal_app_action_show_full_screen (GtkToggleAction *action,
                                      TerminalApp     *app)
{
    toolbar_fs = gtk_toggle_action_get_active (action);
    if (fs == TRUE) {
        terminal_app_set_toolbar (toolbar_fs);  
    }
}

static void
terminal_app_action_show_normal_screen(GtkToggleAction *action,
                                       TerminalApp     *app)
{
    toolbar_normal = gtk_toggle_action_get_active (action);
    if (fs == FALSE) {
        terminal_app_set_toolbar (toolbar_normal);      
    }
}

static void
terminal_app_select_all (GtkAction    *action,
                              TerminalApp  *app)
{
    g_assert (app != NULL);
    g_assert (TERMINAL_IS_APP (app));
    /* terminal_widget_select_all (TERMINAL_WIDGET (app->terminal)); */
    g_debug(__FUNCTION__);
}

#endif
