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

#include "terminal-app.h"
#include "terminal-app-wrapper.h"
#include "shortcuts.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

static void            terminal_app_wrapper_dispose                 (GObject         *object);
static void            terminal_app_wrapper_finalize                (GObject         *object);
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
static void            terminal_app_wrapper_action_close_tab        (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_copy             (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_paste            (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_edit_shortcuts   (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_reverse          (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_fullscreen       (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_scrollbar        (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_toolbar        (GtkToggleAction *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_prev_tab         (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_next_tab         (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_font_size        (GtkRadioAction  *action,
                                                             GtkRadioAction  *current,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_reset            (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_reset_and_clear  (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
static void            terminal_app_wrapper_action_ctrl             (GtkAction       *action,
                                                             TerminalAppWrapper     *app);
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
static void            terminal_app_wrapper_set_toolbar (gboolean show);
static void            terminal_app_wrapper_select_all         (GtkAction       *action,
                                                             TerminalAppWrapper     *app);

static GObjectClass *parent_class;

static GtkActionEntry action_entries[] =
{

  { "file-menu", NULL, N_ ("File"), },
  { "open-url", NULL, N_ ("Open URL"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_open_url), },
  { "close-tab", NULL, N_ ("Close Tab"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_close_tab), },
  { "shortcuts", NULL, N_ ("Shortcuts..."), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_edit_shortcuts), },
  { "go-menu", NULL, N_ ("Go"), },
  { "prev-tab", NULL, N_ ("Previous Tab"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_prev_tab), },
  { "next-tab", NULL, N_ ("Next Tab"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_next_tab), },
  { "font-menu", NULL, N_ ("Font size"), },
  { "terminal-menu", NULL, N_ ("Terminal"), },
  { "reset", NULL, N_ ("Reset"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and Clear"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_reset_and_clear), },
  { "ctrl", NULL, N_ ("Send Ctrl-<some key>"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_ctrl), },
  { "quit", NULL, N_ ("Quit"), NULL, NULL, G_CALLBACK (terminal_app_wrapper_action_quit), },


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


G_DEFINE_TYPE (TerminalAppWrapper, terminal_app, HILDON_TYPE_WINDOW);


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

static void
attach_new_window (TerminalAppWrapper *app, TerminalAppWrapper *toaddapp, 
		   const gchar *label)
{
    GtkWidget *menuitem;
    GtkRadioAction *action = gtk_radio_action_new (label, 
						   label, 
						   NULL, 
						   label, 1);

    gtk_action_group_add_action (app->action_group, GTK_ACTION (action));

    gtk_radio_action_set_group (action, app_window_group);
    app_window_group = gtk_radio_action_get_group (action);

    menuitem = gtk_action_create_menu_item(GTK_ACTION (action));

    gtk_menu_shell_prepend(GTK_MENU_SHELL(toaddapp->windows_menu), menuitem);

    g_signal_connect (menuitem, "activate", G_CALLBACK (on_window_menu_select), app);
    app->menuaction = GTK_ACTION (action);
    g_object_weak_ref (G_OBJECT (app->menuaction), remove_item, menuitem);

}


static void
populate_menubar (TerminalAppWrapper *app, GtkAccelGroup *accelgroup)
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

  GtkWidget *menuitem = gtk_separator_menu_item_new();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(app->windows_menu), 
			menuitem);
  attach_item(app->windows_menu, actiongroup, accelgroup, "new-window");
  app->menubar = menubar;
  gtk_widget_show_all(app->menubar);
}

static int
terminal_app_wrapper_get_font_size(TerminalAppWrapper *app) {
    GtkAction *action;

    return terminal_get_font_size(app->current);

}

static
gboolean terminal_app_wrapper_set_font_size(TerminalAppWrapper *app, int new_size) {

  return terminal_app_wrapper_set_font_size(app, new_size);
}


static gboolean
terminal_app_wrapper_key_press_event (TerminalAppWrapper *app,
                              GdkEventKey *event,
                              gpointer user_data) {

  return terminal_app_wrapper_key_press_event (app, event, user_data);


}

static void
terminal_app_wrapper_init (TerminalAppWrapper *app)
{
  app->current = terminal_app_new ();
  g_slist_appent (app->apps, app->current);

}


static void
terminal_app_wrapper_dispose (GObject *object)
{
  parent_class->dispose (object);
}

static void
terminal_app_wrapper_finalize (GObject *object)
{
  for (GSlist *list = g_slist_nth (app->apps, 0); list != NULL; 
       list = g_slist_next (list)) {
    g_object_unref (list->data);
  }
  parent_class->finalize (object);
}


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


static void
terminal_app_wrapper_action_copy (GtkAction    *action,
                          TerminalAppWrapper  *app)
{
  TerminalWidget *terminal;

  terminal = terminal_app_wrapper_get_active (app);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_copy_clipboard (terminal);
}


static void
terminal_app_wrapper_action_paste (GtkAction    *action,
                           TerminalAppWrapper  *app)
{
  TerminalWidget *terminal;

  terminal = terminal_app_wrapper_get_active (app);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
}

static void
terminal_app_wrapper_action_edit_shortcuts (GtkAction    *action,
                           TerminalAppWrapper  *app)
{
  terminal_app_action_edit_shortcuts (action, app->current);
}


static void
terminal_app_wrapper_action_reverse (GtkToggleAction *action,
                             TerminalAppWrapper     *app)
{
  terminal_app_wrapper_action_reverse (action,
				       app->current);

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
  terminal_app_set_toolbar (show);
}

static void
terminal_app_wrapper_action_toolbar (GtkToggleAction *action,
                               TerminalAppWrapper     *app)
{
    gboolean show;

    show = gtk_toggle_action_get_active (action);
    terminal_app_set_toolbar (show);
    

}

static void
terminal_app_wrapper_action_scrollbar (GtkToggleAction *action,
                               TerminalAppWrapper     *app)
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
terminal_app_wrapper_action_reset (GtkAction   *action,
                           TerminalAppWrapper *app)
{
  TerminalWidget *active;

  active = terminal_app_wrapper_get_active (app);
  terminal_widget_reset (active, FALSE);
}


static void
terminal_app_wrapper_action_reset_and_clear (GtkAction    *action,
                                     TerminalAppWrapper  *app)
{
  TerminalWidget *active;

  active = terminal_app_wrapper_get_active (app);
  terminal_widget_reset (active, TRUE);
}

static void
terminal_app_wrapper_send_ctrl_key(TerminalAppWrapper *app,
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


static void
terminal_app_wrapper_action_settings (GtkAction    *action,
                              TerminalAppWrapper  *app)
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
terminal_app_wrapper_action_quit (GtkAction    *action,
                          TerminalAppWrapper  *app)
{
  //  gtk_widget_destroy(GTK_WIDGET(app));
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
  return g_object_new (TERMINAL_TYPE_APP, NULL);
}


static void
terminal_app_wrapper_real_add (TerminalAppWrapper    *app,
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
  		    G_CALLBACK(terminal_app_wrapper_notify_title), app);
    g_signal_connect (G_OBJECT (widget), "context-menu",
                      G_CALLBACK (terminal_app_wrapper_context_menu), app);
    g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                              G_CALLBACK (terminal_app_wrapper_update_actions), app);
    terminal_app_wrapper_update_actions (app);
    window_list = g_slist_append(window_list, app);

}
/**
 * terminal_app_wrapper_add:
 * @app    : A #TerminalAppWrapper.
 * @widget : A #TerminalWidget.
 **/
void
terminal_app_wrapper_add (TerminalAppWrapper    *app,
                  TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_show (GTK_WIDGET (widget));

  if (windows != 0 ) {
    gpointer newapp = NULL;
    g_debug ("New app");
    newapp = terminal_app_wrapper_new ();
    if (newapp == NULL) {
      g_debug ("Couldn't create new app");
      return;
    }
    g_object_add_weak_pointer(G_OBJECT(newapp), &newapp);
    gtk_widget_show (GTK_WIDGET (newapp));
    terminal_app_wrapper_real_add (newapp, widget);
  } else {
    g_debug ("App");
    terminal_app_wrapper_real_add (app, widget);
  }
  g_object_ref (widget);

  windows++;
}


/**
 * terminal_app_wrapper_remove:
 * @app     :
 * @widget  :
 **/
void
terminal_app_wrapper_remove (TerminalAppWrapper    *app,
                     TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}

/**
 * terminal_app_wrapper_launch
 * @app         : A #TerminalAppWrapper.
 * @error       : Location to store error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_app_wrapper_launch (TerminalAppWrapper     *app,
		     const gchar     *command,
                     GError          **error)
{
  GtkWidget *terminal;
  g_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();
  //g_object_ref (app);
  //g_object_ref (terminal);
  terminal_widget_set_working_directory(TERMINAL_WIDGET(terminal),
		 g_get_home_dir()); 

  terminal_app_wrapper_add (app, TERMINAL_WIDGET (terminal));
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

static void
terminal_app_wrapper_close_window(GtkAction *action, TerminalAppWrapper *app)
{
    g_assert (app);
    g_assert (TERMINAL_IS_APP (app));

    g_debug (__FUNCTION__);

    //gtk_widget_hide (GTK_WIDGET (app));
    //gtk_widget_destroy (GTK_WIDGET (app));
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

