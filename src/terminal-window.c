/* -*- Mode: C; indent-tabs-mode: s; c-basic-offset: 2; tab-width: 2 -*- */
/* vim:set et ai sw=2 ts=2 sts=2: tw=80 cino="(0,W2s,i2s,t0,l1,:0" */
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

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libosso.h>
#include "font-dialog.h"
#include "terminal-gconf.h"

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
#include <gdk/gdkx.h>
#include <vte/vte.h>

#include "terminal-gconf.h"
#include "terminal-settings.h"
#include "terminal-tab-header.h"
#include "terminal-window.h"
#include "terminal-encoding.h"
#include "shortcuts.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

#define FONT_SIZE_INC  2

/* signals */
enum
{
  SIGNAL_NEW_WINDOW = 1,
  LAST_SIGNAL
};

static guint terminal_window_signals[LAST_SIGNAL] = { 0 };

static void            terminal_window_dispose                 (GObject         *object);
static void            terminal_window_finalize                (GObject         *object);
static void            terminal_window_update_actions          (TerminalWindow     *window);
static void            terminal_window_notify_title            (TerminalWidget  *terminal,
                                                             GParamSpec      *pspec,
                                                               TerminalWindow     *window);
static void            terminal_window_action_new_window          (GtkWidget    *new_window_button,
                                                             TerminalWindow     *window);
static void            terminal_window_paste_show            (GtkWidget         *hildon_app_menu,
                                                             TerminalWindow     *window);
static void            terminal_window_action_copy             (GtkButton       *copy_button,
                                                             TerminalWindow     *window);
static void            terminal_window_action_paste            (GtkButton       *paste_button,
		                                                         TerminalWindow     *window);
static void            terminal_window_action_fullscreen       (GtkWidget       * fs_button,
                                                             TerminalWindow     *window);
#if (0)
static void            terminal_window_action_reset            (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reset_and_clear  (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_encoding         (GtkAction       *action,
								TerminalWindow  *window);

#endif /* (0) */
static void            terminal_widget_destroyed (GObject *obj, TerminalWindow *window);

struct _TerminalWindow
{
  HildonWindow __parent__;

  gboolean dispose_has_run;

  GtkWidget *copy_button;
  GtkWidget *paste_button;
  GtkWidget *unfs_button;
  HildonAppMenu *match_menu;

  GConfClient *gconf_client;

  TerminalWidget *terminal;
  gchar *encoding;

  guint take_screenshot_idle_id;
};

static GObjectClass *parent_class;

#if (0)
static GtkActionEntry action_entries[] =
{

  { "reset", NULL, N_ ("Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and Clear"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },

  { "close-all-windows", NULL, N_("weba_me_close_all_windows"), NULL, NULL, G_CALLBACK (terminal_window_action_quit), },

  { "encoding", NULL, N_("weba_fi_encoding_method"), NULL, NULL, G_CALLBACK (terminal_window_action_encoding), },
};

#endif /* (0) */

G_DEFINE_TYPE (TerminalWindow, terminal_window, HILDON_TYPE_WINDOW);

typedef struct {
    GtkWidget *dialog;
    gchar *ret;
} ctrl_dialog_data;

static gboolean
terminal_window_is_fullscreen(TerminalWindow *window)
{
    /* There is no gtk_widget_get_window in maemo gtk lib, so get the window
     * of the parent of one of children. */
    GdkWindowState state;
    state = gdk_window_get_state(
        gtk_widget_get_parent_window(GTK_WIDGET(window->terminal)));
    if((state & GDK_WINDOW_STATE_FULLSCREEN) == GDK_WINDOW_STATE_FULLSCREEN)
      return TRUE;
    return FALSE;
}

static TerminalWidget*
terminal_window_get_active (TerminalWindow *window)
{
  return TERMINAL_WIDGET (window->terminal);    
}

static void
realize(GtkWidget *widget)
{
  void (*parent_realize)(GtkWidget *) =
    GTK_WIDGET_CLASS(g_type_class_peek(g_type_parent(TERMINAL_TYPE_WINDOW)))->realize;

  if (parent_realize)
    parent_realize(widget);

  if (widget->window) {
    unsigned char value = 1;
    Atom hildon_zoom_key_atom = gdk_x11_get_xatom_by_name("_HILDON_ZOOM_KEY_ATOM"),
         integer_atom = gdk_x11_get_xatom_by_name("INTEGER");
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_drawable_get_display(widget->window));
    Window w = GDK_WINDOW_XID(widget->window);

    XChangeProperty(dpy, w, hildon_zoom_key_atom, integer_atom, 8, PropModeReplace, &value, 1);
  }
}

static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;
  GType param_types;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_window_dispose;
  gobject_class->finalize = terminal_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS(klass);
  gtkwidget_class->realize = realize;

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
}

static gboolean
terminal_window_key_press_event (TerminalWindow *window,
                              GdkEventKey *event,
                              gpointer user_data) 
{
    switch (event->keyval) 
    {
        case HILDON_HARDKEY_FULLSCREEN: /* Full screen */
            terminal_window_set_state(window, !terminal_window_is_fullscreen(window));
            return TRUE;

        case HILDON_HARDKEY_INCREASE: /* Zoom in */
          {
            TerminalWidget *tw = terminal_window_get_active(window);
            if (tw) {
              if (!terminal_widget_modify_font_size(tw, FONT_SIZE_INC))
                hildon_banner_show_information(GTK_WIDGET(window), "NULL", _("Already at maximum font size."));
            }
          }
          return TRUE;

        case HILDON_HARDKEY_DECREASE: /* Zoom out */
          {
            TerminalWidget *tw = terminal_window_get_active(window);
            if (tw) {
              if (!terminal_widget_modify_font_size(tw, -FONT_SIZE_INC))
                hildon_banner_show_information(GTK_WIDGET(window), "NULL", _("Already at minimum font size."));
            }
          }
          return TRUE;
    }

    return FALSE;
}

static void
open_match(GtkWidget *btn, TerminalWindow *wnd)
{
  char *match = g_object_get_data(G_OBJECT(wnd->match_menu), "match");

  if (match) {
    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
    if (conn) {
      DBusMessage *msg = dbus_message_new_method_call(OSSO_BROWSER_SERVICE, "/", OSSO_BROWSER_SERVICE, OSSO_BROWSER_OPEN_NEW_WINDOW_REQ);
      if (msg) {
        if (dbus_message_append_args(msg, DBUS_TYPE_STRING, &match, DBUS_TYPE_INVALID)) {
          DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, NULL);
          if (reply)
            dbus_message_unref(reply);
        }
      }
    }
  }
}

static void
copy_match(GtkWidget *btn, TerminalWindow *wnd)
{
  char *match = g_object_get_data(G_OBJECT(wnd->match_menu), "match");

  if (match) {
    GtkClipboard *clipboard = gtk_clipboard_get_for_display(gtk_widget_get_display(btn), GDK_SELECTION_CLIPBOARD);
    if (clipboard)
      gtk_clipboard_set_text(clipboard, match, -1);
    hildon_banner_show_information(GTK_WIDGET(wnd), "NULL", g_dgettext("hildon-common-strings", "ecoc_ib_edwin_copied"));
  }
}

static HildonAppMenu *
make_match_menu(TerminalWindow *wnd)
{
  HildonAppMenu *menu = HILDON_APP_MENU(hildon_app_menu_new());
  GtkButton *btn;

  btn = g_object_new(GTK_TYPE_BUTTON, "visible", TRUE, "label", GTK_STOCK_COPY, "use-stock", TRUE, NULL);
  g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)copy_match, wnd);
  hildon_app_menu_append(menu, btn);

  btn = g_object_new(GTK_TYPE_BUTTON, "visible", TRUE, "label", GTK_STOCK_OPEN, "use-stock", TRUE, NULL);
  g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)open_match, wnd);
  hildon_app_menu_append(menu, btn);

  return menu;
}

static void
terminal_window_init (TerminalWindow *window)
{
  //  GtkWidget           *popup;
  GError              *error = NULL;
  gchar               *role;
  gint                 font_size;
  gboolean             reverse;
  GConfValue          *gconf_value;
  GSList              *keys;
  GSList              *key_labels;
  GtkWidget *hildon_app_menu;
  GtkWidget *button;

  window->dispose_has_run = FALSE;

  window->terminal = NULL;
  window->encoding = NULL;
  window->unfs_button = NULL;
  window->take_screenshot_idle_id = 0;

  gtk_window_set_title(GTK_WINDOW(window), "X Terminal");

  hildon_app_menu = hildon_app_menu_new();

  window->match_menu = g_object_ref_sink(make_match_menu(window));

  /* New window */
  button = g_object_new(GTK_TYPE_BUTTON, "visible", TRUE, "label", GTK_STOCK_NEW, "use-stock", TRUE, NULL);
  g_signal_connect(G_OBJECT(button), "clicked", (GCallback)terminal_window_action_new_window, window);
  hildon_app_menu_append(HILDON_APP_MENU(hildon_app_menu), GTK_BUTTON(button));

  /* Select font */
  button = g_object_new(GTK_TYPE_BUTTON, "visible", TRUE, "label", GTK_STOCK_SELECT_FONT, "use-stock", TRUE, NULL);
  g_signal_connect_data(G_OBJECT(button), "clicked", (GCallback)show_font_dialog, window, NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  hildon_app_menu_append(HILDON_APP_MENU(hildon_app_menu), GTK_BUTTON(button));

  /* Copy */
  window->copy_button = g_object_new(GTK_TYPE_BUTTON, "visible", TRUE, "label", GTK_STOCK_COPY, "use-stock", TRUE, NULL);
  g_object_ref_sink(window->copy_button);
  g_signal_connect(G_OBJECT(window->copy_button), "clicked", (GCallback)terminal_window_action_copy, window);
  hildon_app_menu_append(HILDON_APP_MENU(hildon_app_menu), GTK_BUTTON(window->copy_button));

  /* Paste */
  window->paste_button = g_object_new(GTK_TYPE_BUTTON, "visible", TRUE, "label", GTK_STOCK_PASTE, "use-stock", TRUE, NULL);
  g_object_ref_sink(window->paste_button);
  g_signal_connect(G_OBJECT(window->paste_button), "clicked", (GCallback)terminal_window_action_paste, window);
  g_signal_connect(G_OBJECT(hildon_app_menu), "show", (GCallback)terminal_window_paste_show, window);
  hildon_app_menu_append(HILDON_APP_MENU(hildon_app_menu), GTK_BUTTON(window->paste_button));

  hildon_window_set_app_menu(HILDON_WINDOW(window), HILDON_APP_MENU(hildon_app_menu));

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
  TerminalWindow *window = TERMINAL_WINDOW (object);

  if(window->dispose_has_run)
    return;
  window->dispose_has_run = TRUE;

  if (window->terminal != NULL){
    g_object_unref (window->terminal);
    window->terminal = NULL;
  }

  g_object_unref(window->copy_button);
  window->copy_button = NULL;
  g_object_unref(window->paste_button);
  window->paste_button = NULL;
  g_object_unref(window->unfs_button);
  window->unfs_button = NULL;
  g_object_unref(window->match_menu);
  window->match_menu = NULL;

  if (window->take_screenshot_idle_id) {
    g_source_remove(window->take_screenshot_idle_id);
    window->take_screenshot_idle_id = 0;
  }

  parent_class->dispose (object);
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  g_object_unref(G_OBJECT(window->gconf_client));

  g_free(window->encoding);
  window->encoding = NULL;

  parent_class->finalize (object);
}


static void
terminal_window_update_actions (TerminalWindow *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      g_object_set (G_OBJECT (window->copy_button),
                    "visible", terminal_widget_has_selection (terminal),
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
terminal_window_action_new_window (GtkWidget *new_window_button,
                             TerminalWindow  *window)
{
  /* new-window signal */
  g_signal_emit (G_OBJECT (window), 
                 terminal_window_signals[SIGNAL_NEW_WINDOW], 0, NULL); 

}

/* Check is paste enabled */
static void
terminal_window_paste_show (GtkWidget *hildon_app_menu,
                             TerminalWindow *window)
{
  gboolean paste_enabled = 
    gtk_clipboard_wait_is_text_available (gtk_clipboard_get (GDK_NONE));

  g_object_set (G_OBJECT (window->paste_button), "visible", paste_enabled, NULL);
}

static void
terminal_window_action_copy (GtkButton    *copy_button,
                          TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL)) {
    terminal_widget_copy_clipboard (terminal);
    hildon_banner_show_information(GTK_WIDGET(window), "NULL", g_dgettext("hildon-common-strings", "ecoc_ib_edwin_copied"));
  }
}


static void
terminal_window_action_paste (GtkButton    *paste_button,
                           TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
}

static void
terminal_window_action_fullscreen (GtkWidget *fs_button,
                                TerminalWindow     *window)
{
  gboolean fs;
  g_object_get(G_OBJECT(fs_button), "active", &fs, NULL);
  terminal_window_set_state(window, fs);
}

#if (0)
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
#endif /* (0) */

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
terminal_widget_destroyed (GObject *obj, TerminalWindow *window)
{
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  g_assert (TERMINAL_IS_WINDOW (window));
  if(window->terminal != NULL){
    g_object_unref(window->terminal);
    window->terminal = NULL;
    gtk_widget_destroy (GTK_WIDGET (window));
  }
}

static void
notify_match(GtkWidget *widget, GParamSpec *pspec, TerminalWindow *wnd)
{
  char *match = NULL;

  g_object_get(G_OBJECT(widget), "match", &match, NULL);
  if (match) {
    g_object_set_data_full(G_OBJECT(wnd->match_menu), "match", match, (GDestroyNotify)g_free);
    hildon_app_menu_popup(wnd->match_menu, GTK_WINDOW(wnd));
  }
}

static gboolean
maybe_take_screenshot(TerminalWindow *window)
{
  if (!g_file_test(OSSO_XTERM_SCREENSHOT_FILE_NAME, G_FILE_TEST_EXISTS))
    hildon_gtk_window_take_screenshot(GTK_WINDOW(window), TRUE);
  window->take_screenshot_idle_id = 0;

  return FALSE;
}

static void
terminal_window_real_add (
    TerminalWindow *window,
    TerminalWidget *widget)
{
  /*
    gchar *title = terminal_widget_get_title(widget);
    g_object_set(GTK_WINDOW (window), "title", title, NULL);
    g_free(title);
  */
    gtk_container_add(GTK_CONTAINER (window), GTK_WIDGET(widget));

    /* add terminal to window */
    window->terminal = g_object_ref_sink(widget);

    g_signal_connect(G_OBJECT(widget), "notify::title",
  		    G_CALLBACK(terminal_window_notify_title), window);
    g_signal_connect (G_OBJECT (widget), "destroy",
                      G_CALLBACK (terminal_widget_destroyed), window);
    g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                              G_CALLBACK (terminal_window_update_actions), window);
    terminal_window_update_actions (window);
    g_signal_connect(G_OBJECT(widget->terminal), "notify::match", (GCallback)notify_match, window);

    window->unfs_button = GTK_WIDGET(
        g_object_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
                     "visible", TRUE,
                     "active", FALSE,
                     "icon-widget", g_object_new(GTK_TYPE_IMAGE,
                                                 "visible", TRUE,
                                                 "icon-name", "general_fullsize",
                                                 "icon-size", HILDON_ICON_SIZE_TOOLBAR,
                                                 NULL),
                     NULL));
    g_object_ref_sink(window->unfs_button);
    g_signal_connect(G_OBJECT(window->unfs_button), "toggled", (GCallback)terminal_window_action_fullscreen, window);
    terminal_widget_add_tool_item(TERMINAL_WIDGET(widget), GTK_TOOL_ITEM(window->unfs_button));

  window->take_screenshot_idle_id = g_idle_add((GSourceFunc)maybe_take_screenshot, window);
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

  gtk_widget_destroy (GTK_WIDGET (widget));
}

/**
 * terminal_window_launch
 * @window         : A #TerminalWindow.
 * @error       : Location to store error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_window_launch (
    TerminalWindow *window,
    const gchar *command,
    GError **error)
{
  gboolean child_launched = FALSE;
  GtkWidget *terminal;

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();
  terminal_widget_set_working_directory(TERMINAL_WIDGET(terminal),
		 g_get_home_dir());
  terminal_widget_set_app_win (TERMINAL_WIDGET (terminal), HILDON_WINDOW (window));

  gtk_widget_show (GTK_WIDGET (terminal));
  terminal_window_real_add (window, TERMINAL_WIDGET(terminal));
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

  child_launched = terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

  if (child_launched) {
    gtk_widget_show_all(GTK_WIDGET(window));

    if (window->encoding == NULL) {
      gconf_client_set_string(window->gconf_client, OSSO_XTERM_GCONF_ENCODING, 
			      OSSO_XTERM_DEFAULT_ENCODING, NULL);
      g_object_set (window->terminal, "encoding", 
		    OSSO_XTERM_DEFAULT_ENCODING, NULL);
    } else {
      g_object_set (window->terminal, "encoding", window->encoding, NULL);
    }

  /*  g_idle_add ((GSourceFunc)_im_context_focus, window->terminal); */
  }
  return child_launched;
}

void terminal_window_set_state (TerminalWindow *window, gboolean go_fs)
{
    gboolean fs = terminal_window_is_fullscreen(window);

    if(go_fs){
      if(!fs){
        gtk_window_fullscreen(GTK_WINDOW(window));
        terminal_widget_update_tool_bar(
            window->terminal, terminal_widget_need_toolbar(window->terminal));
      }
    } else {
      if(fs){
        gtk_window_unfullscreen(GTK_WINDOW(window));
        terminal_widget_update_tool_bar(
            window->terminal, terminal_widget_need_fullscreen_toolbar(window->terminal));
      }
    }
}
