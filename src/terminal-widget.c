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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#define FONT_SIZE_MAX_ABS_DELTA 8

#define GETTEXT_PACKAGE "osso-browser-ui"
#include <glib/gi18n-lib.h>
/*
  #include <libintl.h>
  #include <locale.h>
  #define _(String) gettext(String)
*/

#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include "maemo-vte.h"

#include "terminal-gconf.h"
#include "terminal-widget.h"

#include <hildon/hildon.h>

#define TERMINAL_WIDGET_STOCK_PAN        PACKAGE "-pan"
#define TERMINAL_WIDGET_STOCK_DO_NOT_PAN PACKAGE "-do-not-pan"

enum
  {
    PROP_0,
    PROP_CUSTOM_TITLE,
    PROP_ENCODING,
    PROP_TITLE,
  };

enum
  {
    CONTEXT_MENU,
    SELECTION_CHANGED,
    LAST_SIGNAL,
  };


static void     terminal_widget_dispose                       (GObject         *object);
static void     terminal_widget_finalize                      (GObject          *object);
static void     terminal_widget_get_property                  (GObject          *object,
                                                               guint             prop_id,
                                                               GValue           *value,
                                                               GParamSpec       *pspec);
static void     terminal_widget_set_property                  (GObject          *object,
                                                               guint             prop_id,
                                                               const GValue     *value,
                                                               GParamSpec       *pspec);
static gboolean terminal_widget_get_child_command             (TerminalWidget   *widget,
                                                               gchar           **command,
                                                               gchar          ***argv,
                                                               GError          **error);
static gchar  **terminal_widget_get_child_environment         (TerminalWidget   *widget);
#if 0
static void terminal_widget_update_background                 (TerminalWidget   *widget);
#endif
static void terminal_widget_update_binding_backspace          (TerminalWidget   *widget);
static void terminal_widget_update_binding_delete             (TerminalWidget   *widget);
static void terminal_widget_update_misc_bell                  (TerminalWidget   *widget);
static void terminal_widget_update_misc_cursor_blinks         (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_lines            (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_on_output        (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_on_keystroke     (TerminalWidget   *widget);
#if 0
static void terminal_widget_update_title                      (TerminalWidget   *widget);
#endif
static void terminal_widget_update_word_chars                 (TerminalWidget   *widget);
static void terminal_widget_vte_child_exited                  (VteTerminal      *terminal,
                                                               TerminalWidget   *widget);
static void terminal_widget_vte_drag_data_received            (VteTerminal      *terminal,
                                                               GdkDragContext   *context,
                                                               gint              x,
                                                               gint              y,
                                                               GtkSelectionData *selection_data,
                                                               guint             info,
                                                               guint             time,
                                                               TerminalWidget   *widget);
static void     terminal_widget_vte_encoding_changed          (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_eof                       (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_vte_button_press_event        (VteTerminal    *terminal,
                                                               GdkEventButton *event,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_vte_key_press_event           (VteTerminal    *terminal,
                                                               GdkEventKey    *event,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_realize                   (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_selection_changed         (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_window_title_changed      (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_timer_background              (gpointer        user_data);
static void     terminal_widget_gconf_toolbar               (GConfClient    *client,
							     guint           conn_id,
							     GConfEntry     *entry,
							     TerminalWidget *widget);
static void     terminal_widget_gconf_toolbar_fs            (GConfClient    *client,
							     guint           conn_id,
							     GConfEntry     *entry,
							     TerminalWidget *widget);
static void     terminal_widget_gconf_keys               (GConfClient    *client,
							  guint           conn_id,
							  GConfEntry     *entry,
							  TerminalWidget *widget);
static void     terminal_widget_gconf_reverse                 (GConfClient    *client,
                                                               guint           conn_id,
                                                               GConfEntry     *entry,
                                                               TerminalWidget *widget);
static void     terminal_widget_gconf_font_size               (GConfClient    *client,
                                                               guint           conn_id,
                                                               GConfEntry     *entry,
                                                               TerminalWidget *widget);
static void     terminal_widget_update_font                   (TerminalWidget *widget,
							       const gchar *name,
                                                               gint size);
static void     terminal_widget_update_keys          (TerminalWidget *widget,
						      GSList *keys,
						      GSList *key_labels);
static void     terminal_widget_update_colors		      (TerminalWidget *widget,
							       const gchar *fg_name,
							       const gchar *bg_name,
                                                               gboolean reverse);
#if 0
static void     terminal_widget_timer_background_destroy      (gpointer        user_data);
#endif
static void     terminal_widget_emit_context_menu            (TerminalWidget *widget,
		                                              gpointer user_data);
static void	terminal_widget_ctrlify_notify	     	     (GObject *src, GParamSpec *pspec, GObject *dst);
static void	terminal_widget_do_keys			     (TerminalWidget *widget,
							      const gchar *key_string);
static void	terminal_widget_do_key_button		     (GObject *button,
							      TerminalWidget *widget);
#if 0
static void terminal_widget_ctrl_clicked (GtkButton    *item,
					  TerminalWidget *widget);
#endif
static GObjectClass *parent_class;
static guint widget_signals[LAST_SIGNAL];

enum
  {
    TARGET_URI_LIST,
    TARGET_UTF8_STRING,
    TARGET_TEXT,
    TARGET_COMPOUND_TEXT,
    TARGET_STRING,
    TARGET_TEXT_PLAIN,
    TARGET_MOZ_URL,
  };

static GtkTargetEntry target_table[] =
  {
    { (gchar*)"text/uri-list", 0, TARGET_URI_LIST },
    { (gchar*)"text/x-moz-url", 0, TARGET_MOZ_URL },
    { (gchar*)"UTF8_STRING", 0, TARGET_UTF8_STRING },
    { (gchar*)"TEXT", 0, TARGET_TEXT },
    { (gchar*)"COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT },
    { (gchar*)"STRING", 0, TARGET_STRING },
    { (gchar*)"text/plain", 0, TARGET_TEXT_PLAIN },
  };


G_DEFINE_TYPE (TerminalWidget, terminal_widget, GTK_TYPE_VBOX);

static void
terminal_widget_class_init (TerminalWidgetClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_widget_dispose;
  gobject_class->finalize = terminal_widget_finalize;
  gobject_class->get_property = terminal_widget_get_property;
  gobject_class->set_property = terminal_widget_set_property;

  /**
   * TerminalWidget:custom-title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_TITLE,
                                   g_param_spec_string ("custom-title",
                                                        _("Custom title"),
                                                        _("Custom title"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalWidget:encoding:
   *
   * The encoding the terminal will expect data from the child to be encoded
   * with. For certain terminal types, applications executing in the terminal
   * can change the encoding. The default encoding is defined by the application's
   * locale settings.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ENCODING,
                                   g_param_spec_string ("encoding",
                                                        _("Encoding"),
                                                        _("Terminal encoding"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalWidget:title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        _("Title"),
                                                        _("Title"),
                                                        NULL,
                                                        G_PARAM_READABLE));

  /**
   * TerminalWidget::context-menu
   **/
  widget_signals[CONTEXT_MENU] =
    g_signal_new ("context-menu",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, context_menu),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  /**
   * TerminalWidget::selection-changed
   **/
  widget_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
maybe_set_pan_mode(TerminalWidget *terminal_widget, GParamSpec *pspec, GObject *src)
{
  g_assert(G_OBJECT(terminal_widget));
  GObject *pan_btn_obj = G_OBJECT(terminal_widget->pan_button);
  GObject *mvte_obj = G_OBJECT(terminal_widget->terminal);
  GtkAdjustment *adj = vte_terminal_get_adjustment(VTE_TERMINAL(terminal_widget->terminal));
  gboolean is_active, bt_pan_visible, is_pan_mode,
           can_pan = (adj->upper - adj->page_size > adj->lower);

/*  g_printerr("%s : "
                  "upper: %f, lower: %f, page_size: %f, value: %f, step_inc: %f, page_inc: %f\n",
                  __FUNCTION__, adj->upper, adj->lower, adj->page_size, adj->value, adj->step_increment, adj->page_increment);
*/
  g_object_freeze_notify(pan_btn_obj);

  g_object_get(pan_btn_obj, "visible", &bt_pan_visible, "active", &is_active, NULL);
  g_object_get(mvte_obj, "pan-mode", &is_pan_mode, NULL);

  if (bt_pan_visible != can_pan)
    g_object_set(pan_btn_obj, "visible", can_pan, NULL);

  if (can_pan) {
    if (is_pan_mode != !is_active) {
      g_object_set(mvte_obj, "pan-mode", !is_active, NULL);
      gtk_widget_show(GTK_WIDGET(pan_btn_obj));

      g_object_set(pan_btn_obj,
          "icon-widget", g_object_new(GTK_TYPE_IMAGE,
                                      "visible", TRUE,
                                      "icon-name", is_active ? "browser_panning_mode_on" : "browser_panning_mode_off",
                                      "icon-size", HILDON_ICON_SIZE_TOOLBAR,
                                      NULL),
          NULL);
    }
  }
  else {
    if (is_pan_mode)
      g_object_set(mvte_obj, "pan-mode", FALSE, NULL);
    gtk_widget_hide(GTK_WIDGET(pan_btn_obj));
  }

  g_object_thaw_notify(pan_btn_obj);
}

gboolean
terminal_widget_need_toolbar(TerminalWidget *widget)
{
  gboolean toolbar;
  GConfValue *gconf_value;
  GError *err = NULL;

  gconf_value = gconf_client_get(widget->gconf_client,
                                 OSSO_XTERM_GCONF_TOOLBAR,
                                 &err);
  if (err != NULL) {
    g_printerr("Unable to get toolbar setting for non-fullscreen from gconf: %s\n",
	       err->message);
    g_clear_error(&err);
  }
  toolbar = OSSO_XTERM_DEFAULT_TOOLBAR;
  if (gconf_value) {
    if (gconf_value->type == GCONF_VALUE_BOOL)
      toolbar = gconf_value_get_bool(gconf_value);
    gconf_value_free(gconf_value);
  }

  return toolbar;
}

gboolean
terminal_widget_need_fullscreen_toolbar(TerminalWidget *widget)
{
  gboolean toolbar;
  GConfValue *gconf_value;
  GError *err = NULL;

  gconf_value = gconf_client_get(widget->gconf_client,
                                 OSSO_XTERM_GCONF_TOOLBAR_FULLSCREEN,
                                 &err);
  if (err != NULL) {
    g_printerr("Unable to get toolbar setting for fullscreen from gconf: %s\n",
	       err->message);
    g_clear_error(&err);
  }
  toolbar = OSSO_XTERM_DEFAULT_TOOLBAR_FULLSCREEN;
  if (gconf_value) {
    if (gconf_value->type == GCONF_VALUE_BOOL)
      toolbar = gconf_value_get_bool(gconf_value);
    gconf_value_free(gconf_value);
  }

  return toolbar;
}

static void
terminal_widget_init (TerminalWidget *widget)
{
  GError *err = NULL;
  GSList *keys;
  GSList *key_labels;
  GtkWidget *hbox;
  GRegex *re[4];
  gint i;

  widget->dispose_has_run = FALSE;

  widget->working_directory = g_get_current_dir ();
  widget->custom_title = g_strdup ("");

  widget->gconf_client = gconf_client_get_default ();

  gconf_client_add_dir(widget->gconf_client,
		       OSSO_XTERM_GCONF_PATH,
		       GCONF_CLIENT_PRELOAD_NONE,
		       &err); /* err */
  if (err != NULL) {
    g_printerr("gconf_client_add_dir(): %s\n", err->message);
    g_clear_error(&err);
  }

  widget->toolbar_conid = gconf_client_notify_add(widget->gconf_client,
	  OSSO_XTERM_GCONF_TOOLBAR,
	  (GConfClientNotifyFunc)terminal_widget_gconf_toolbar,
	  widget,
	  NULL, &err);
  widget->toolbar_fs_conid = gconf_client_notify_add(widget->gconf_client,
	  OSSO_XTERM_GCONF_TOOLBAR_FULLSCREEN,
	  (GConfClientNotifyFunc)terminal_widget_gconf_toolbar_fs,
	  widget,
	  NULL, &err);
  widget->keys_conid = gconf_client_notify_add(widget->gconf_client,
	  OSSO_XTERM_GCONF_KEYS,
	  (GConfClientNotifyFunc)terminal_widget_gconf_keys,
	  widget,
	  NULL, &err);
  widget->key_labels_conid = gconf_client_notify_add(widget->gconf_client,
	  OSSO_XTERM_GCONF_KEY_LABELS,
	  (GConfClientNotifyFunc)terminal_widget_gconf_keys,
	  widget,
	  NULL, &err);
  if (err != NULL) {
    g_clear_error(&err);
  }

  widget->font_size_conid = gconf_client_notify_add(widget->gconf_client,
						    OSSO_XTERM_GCONF_FONT_SIZE,
						    (GConfClientNotifyFunc)terminal_widget_gconf_font_size,
						    widget,
						    NULL, &err);
  if (err != NULL) {
    g_printerr("font_size notify add failed: %s\n", err->message);
    g_clear_error(&err);
  }

  widget->font_base_size_conid = gconf_client_notify_add(widget->gconf_client,
							 OSSO_XTERM_GCONF_FONT_BASE_SIZE,
							 (GConfClientNotifyFunc)terminal_widget_gconf_font_size,
							 widget,
							 NULL, &err);
  if (err != NULL) {
    g_printerr("font_size notify add failed: %s\n", err->message);
    g_clear_error(&err);
  }

  widget->font_name_conid = gconf_client_notify_add(widget->gconf_client,
						    OSSO_XTERM_GCONF_FONT_NAME,
						    (GConfClientNotifyFunc)terminal_widget_gconf_font_size,
						    widget,
						    NULL, &err);
  if (err != NULL) {
    g_printerr("font_size notify add failed: %s\n", err->message);
    g_clear_error(&err);
  }

  widget->fg_conid = gconf_client_notify_add(widget->gconf_client,
					     OSSO_XTERM_GCONF_FONT_COLOR,
					     (GConfClientNotifyFunc)terminal_widget_gconf_reverse,
					     widget,
					     NULL, &err);
  if (err != NULL) {
    g_printerr("reverse notify add failed: %s\n", err->message);
    g_clear_error(&err);
  }
  widget->bg_conid = gconf_client_notify_add(widget->gconf_client,
					     OSSO_XTERM_GCONF_BG_COLOR,
					     (GConfClientNotifyFunc)terminal_widget_gconf_reverse,
					     widget,
					     NULL, &err);
  if (err != NULL) {
    g_printerr("reverse notify add failed: %s\n", err->message);
    g_clear_error(&err);
  }
  widget->reverse_conid = gconf_client_notify_add(widget->gconf_client,
						  OSSO_XTERM_GCONF_REVERSE,
						  (GConfClientNotifyFunc)terminal_widget_gconf_reverse,
						  widget,
						  NULL, &err);
  if (err != NULL) {
    g_printerr("reverse notify add failed: %s\n", err->message);
    g_clear_error(&err);
  }

  widget->keys_toolbuttons = NULL;

  widget->terminal = g_object_new(MAEMO_VTE_TYPE, "pan-mode", TRUE, NULL);
  g_object_ref_sink(widget->terminal);

  g_signal_connect (G_OBJECT (widget->terminal), "child-exited",
                    G_CALLBACK (terminal_widget_vte_child_exited), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "encoding-changed",
                    G_CALLBACK (terminal_widget_vte_encoding_changed), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "eof",
                    G_CALLBACK (terminal_widget_vte_eof), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "button-press-event",
                    G_CALLBACK (terminal_widget_vte_button_press_event), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "key-press-event",
                    G_CALLBACK (terminal_widget_vte_key_press_event), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "selection-changed",
                    G_CALLBACK (terminal_widget_vte_selection_changed), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "realize",
                    G_CALLBACK (terminal_widget_vte_realize), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "window-title-changed",
                    G_CALLBACK (terminal_widget_vte_window_title_changed), widget);

  hbox = g_object_new(HILDON_TYPE_PANNABLE_AREA,
    "drag-inertia", 0.1,
    "hovershoot-max", 0,
    "vovershoot-max", 0,
    "child", widget->terminal,
    NULL);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX (widget), hbox, TRUE, TRUE, 0);
  gtk_widget_show (widget->terminal);

  /* setup Drag'n'Drop support */
  g_signal_connect (G_OBJECT (widget->terminal), "drag-data-received",
                    G_CALLBACK (terminal_widget_vte_drag_data_received), widget);
  gtk_drag_dest_set (GTK_WIDGET (widget->terminal),
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     target_table, G_N_ELEMENTS (target_table),
                     GDK_ACTION_COPY);

  gtk_widget_grab_focus(widget->terminal);

  widget->tbar = g_object_ref_sink(gtk_toolbar_new());
  g_object_set(widget->tbar, 
	       "orientation", GTK_ORIENTATION_HORIZONTAL,
	       NULL);
  gtk_widget_show (GTK_WIDGET (widget->tbar));

  widget->cbutton = g_object_ref_sink(gtk_toggle_tool_button_new());
  gtk_tool_item_set_expand(widget->cbutton, FALSE);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget->cbutton), "Ctrl");
  gtk_widget_show(GTK_WIDGET(widget->cbutton));

  widget->pan_button = g_object_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
      "icon-widget", g_object_new(GTK_TYPE_IMAGE,
                                  "visible", TRUE,
                                  "icon-name", "browser_panning_mode_off",
                                  "icon-size", HILDON_ICON_SIZE_TOOLBAR,
                                  NULL),
      NULL);
  g_object_ref_sink(widget->pan_button);

  gtk_tool_item_set_expand(widget->pan_button, FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(widget->tbar), widget->pan_button, -1);

  g_signal_connect_swapped(G_OBJECT(vte_terminal_get_adjustment(VTE_TERMINAL(widget->terminal))), "changed", (GCallback)maybe_set_pan_mode, widget);
  g_signal_connect_swapped(G_OBJECT(widget->pan_button), "notify::active", (GCallback)maybe_set_pan_mode, widget);
  g_signal_connect_swapped(G_OBJECT(widget->pan_button), "notify::visible", (GCallback)maybe_set_pan_mode, widget);

#ifdef DEBUG
  g_debug("%s - tbar: %p", __FUNCTION__, widget->tbar);
#endif

  /* Link these two properties */
  g_signal_connect (G_OBJECT(widget->terminal), "notify::control-mask",
		    G_CALLBACK(terminal_widget_ctrlify_notify),
		    widget->cbutton);
  g_signal_connect (G_OBJECT(widget->cbutton), "notify::active",
    G_CALLBACK(terminal_widget_ctrlify_notify),
    widget->terminal);

 
  /*  g_signal_connect (G_OBJECT(widget->cbutton), "clicked",
		    G_CALLBACK(terminal_widget_ctrl_clicked),
		    widget);
  */
  gtk_toolbar_insert(GTK_TOOLBAR(widget->tbar),
		     widget->cbutton,
		     -1);
  //  gtk_box_pack_start (GTK_BOX (widget), widget->tbar, FALSE, FALSE, 0);

  /* apply current settings */

  keys = gconf_client_get_list(widget->gconf_client,
			       OSSO_XTERM_GCONF_KEYS,
			       GCONF_VALUE_STRING,
			       &err);
  key_labels = gconf_client_get_list(widget->gconf_client,
				     OSSO_XTERM_GCONF_KEY_LABELS,
				     GCONF_VALUE_STRING,
				     &err);

  terminal_widget_update_tool_bar(widget, terminal_widget_need_toolbar(widget));
  terminal_widget_update_keys(TERMINAL_WIDGET(widget), keys, key_labels);

  g_slist_foreach(keys, (GFunc)g_free, NULL);
  g_slist_foreach(key_labels, (GFunc)g_free, NULL);
  g_slist_free(keys);
  g_slist_free(key_labels);

  terminal_widget_gconf_font_size(widget->gconf_client, 0, NULL, widget);
  terminal_widget_gconf_reverse (widget->gconf_client, 0, NULL, widget);

  terminal_widget_update_binding_backspace (widget);
  terminal_widget_update_binding_delete (widget);
  terminal_widget_update_misc_bell (widget);
  terminal_widget_update_misc_cursor_blinks (widget);
  terminal_widget_update_scrolling_lines (widget);
  terminal_widget_update_scrolling_on_output (widget);
  terminal_widget_update_scrolling_on_keystroke (widget);
  terminal_widget_update_word_chars (widget);

#define USERCHARS "-A-Za-z0-9"
#define PASSCHARS "-A-Za-z0-9,?;.:/!%$^*&~\"#'"
#define HOSTCHARS "-A-Za-z0-9"
#define PATHCHARS "-A-Za-z0-9_$.+!*(),;:@&=?/~#%"
#define SCHEME    "(news:|telnet:|nntp:|file:/|https?:|ftps?:|webcal:)"
#define USER      "[" USERCHARS "]+(:["PASSCHARS "]+)?"
#define URLPATH   "/[" PATHCHARS "]*[^]'.}>) \t\r\n,\\\"]"

  re[0] = g_regex_new("\\b" SCHEME "//(" USER "@)?[" HOSTCHARS ".]+(:[0-9]+)?("
		       URLPATH ")?\\b", 0, 0, NULL);

  re[1] = g_regex_new("\\b(www|ftp)[" HOSTCHARS "]*\\.[" HOSTCHARS ".]+"
		      "(:[0-9]+)?(" URLPATH ")?\\b", 0, 0, NULL);

  re[2] = g_regex_new("\\b(mailto:)?[a-z0-9][a-z0-9.-]*@[a-z0-9][a-z0-9-]*"
		      "(\\.[a-z0-9][a-z0-9-]*)+\\b", 0, 0, NULL);

  re[3] = g_regex_new("\\bnews:[-A-Z\\^_a-z{|}~!\"#$%&'()*+,./0-9;:=?`]+"
		      "@[" HOSTCHARS ".]+(:[0-9]+)?\\b",
		      0, 0, NULL);

  for (i = 0; i < 4; i++) {
    vte_terminal_match_add_gregex (VTE_TERMINAL(widget->terminal), re[i], 0);
    g_regex_unref (re[i]);
  }

  gtk_widget_tap_and_hold_setup (GTK_WIDGET(widget->terminal), NULL, NULL,
				 GTK_TAP_AND_HOLD_NONE);
  g_signal_connect_swapped(G_OBJECT(widget->terminal), "tap-and-hold",
		           G_CALLBACK(terminal_widget_emit_context_menu),
			   widget);
}

static void
terminal_widget_dispose (GObject *object)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  if(widget->dispose_has_run)
    return;
  widget->dispose_has_run = TRUE;

  /* disconnect signals from keys toolbar buttons */
  for(GSList *iter = widget->keys_toolbuttons;
      iter != NULL; iter = iter->next){
    g_signal_handlers_disconnect_by_func(iter->data,
        terminal_widget_do_key_button, widget);
    g_object_unref(iter->data);
    iter->data = NULL;
  }
  widget->keys_toolbuttons = g_slist_remove_all(
      widget->keys_toolbuttons, NULL);

  g_signal_handlers_disconnect_by_func(widget->cbutton,
      terminal_widget_ctrlify_notify, widget->terminal);
/*  g_signal_handlers_disconnect_by_func(widget->cbutton,
		  terminal_widget_ctrlify_notify, widget);*/

  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_child_exited, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_encoding_changed, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_eof, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_button_press_event, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_key_press_event, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_selection_changed, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_realize, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
    terminal_widget_vte_window_title_changed, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
      terminal_widget_ctrlify_notify, widget->cbutton);
  g_signal_handlers_disconnect_by_func(widget->terminal,
		  terminal_widget_emit_context_menu, widget);
  g_signal_handlers_disconnect_by_func(widget->terminal,
      terminal_widget_vte_drag_data_received, widget);
  g_signal_handlers_disconnect_by_func(
      vte_terminal_get_adjustment(VTE_TERMINAL(widget->terminal)),
      maybe_set_pan_mode, widget);

  g_signal_handlers_disconnect_by_func(widget->pan_button,
      maybe_set_pan_mode, widget);

  g_object_unref(widget->pan_button);
  widget->pan_button = NULL;
  g_object_unref(widget->cbutton);
  widget->cbutton = NULL;
  g_object_unref(widget->terminal);
  widget->terminal = NULL;
  g_object_unref(widget->tbar);
  widget->tbar = NULL;


  parent_class->dispose (object);
}

static void
terminal_widget_finalize (GObject *object)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  g_free (widget->working_directory);
  g_strfreev (widget->custom_command);
  g_free (widget->custom_title);

  gconf_client_notify_remove(widget->gconf_client,
                             widget->toolbar_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->toolbar_fs_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->keys_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->key_labels_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->font_name_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->font_base_size_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->font_size_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->fg_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->bg_conid);
  gconf_client_notify_remove(widget->gconf_client,
                             widget->reverse_conid);
  gconf_client_remove_dir(widget->gconf_client,
			  OSSO_XTERM_GCONF_PATH,
			  NULL);

  g_object_unref(G_OBJECT(widget->gconf_client));

  /**/

  parent_class->finalize (object);
}


static void
terminal_widget_get_property (GObject          *object,
                              guint             prop_id,
                              GValue           *value,
                              GParamSpec       *pspec)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_CUSTOM_TITLE:
      g_value_set_string (value, widget->custom_title);
      break;

    case PROP_ENCODING:
      g_value_set_string (value, vte_terminal_get_encoding (VTE_TERMINAL (widget->terminal)));
      break;

    case PROP_TITLE:
      g_value_take_string (value, terminal_widget_get_title (widget));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
terminal_widget_set_property (GObject          *object,
                              guint             prop_id,
                              const GValue     *value,
                              GParamSpec       *pspec)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_CUSTOM_TITLE:
      terminal_widget_set_custom_title (widget, g_value_get_string (value));
      break;

    case PROP_ENCODING:
      vte_terminal_set_encoding (VTE_TERMINAL (widget->terminal), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
terminal_widget_get_child_command (TerminalWidget   *widget,
                                   gchar           **command,
                                   gchar          ***argv,
                                   GError          **error)
{
  struct passwd *pw;
  const gchar   *shell_name;

  if (widget->custom_command != NULL)
    {
      *command = g_strdup (widget->custom_command[0]);
      *argv    = g_strdupv (widget->custom_command);
    }
  else
    {
      pw = getpwuid (getuid ());
      if (G_UNLIKELY (pw == NULL))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                       _("Current user unknown"));
          return FALSE;
        }

      shell_name = strrchr (pw->pw_shell, '/');
      if (shell_name != NULL)
        ++shell_name;
      else
        shell_name = pw->pw_shell;
      *command = g_strdup (pw->pw_shell);

      *argv = g_new (gchar *, 2);
      (*argv)[0] = g_strconcat ("-", shell_name, NULL);
      (*argv)[1] = NULL;
    }

  return TRUE;
}


static gchar**
terminal_widget_get_child_environment (TerminalWidget *widget)
{
  extern gchar    **environ;
  gchar           **result;
  gchar           **p;
  guint             n;

  /* count env vars that are set */
  for (p = environ; *p != NULL; ++p);

  n = p - environ;
  result = g_new (gchar *, n + 1 + 4);

  for (n = 0, p = environ; *p != NULL; ++p)
    {
      if ((strncmp (*p, "COLUMNS=", 8) == 0)
          || (strncmp (*p, "LINES=", 6) == 0)
          || (strncmp (*p, "WINDOWID=", 9) == 0)
          || (strncmp (*p, "TERM=", 5) == 0)
          || (strncmp (*p, "GNOME_DESKTOP_ICON=", 19) == 0)
          || (strncmp (*p, "COLORTERM=", 10) == 0)
          || (strncmp ( *p, "DISPLAY=", 8) == 0))
        {
          /* nothing: do not copy */
        }
      else
        {
          result[n] = g_strdup (*p);
          ++n;
        }
    }

  result[n++] = g_strdup ("COLORTERM=Terminal");

  if (GTK_WIDGET_REALIZED (widget->terminal))
    {
      result[n++] = g_strdup_printf ("WINDOWID=%ld", (glong) GDK_WINDOW_XWINDOW (widget->terminal->window));
      result[n++] = g_strdup_printf ("DISPLAY=%s", gdk_display_get_name (gtk_widget_get_display (widget->terminal)));
    }

  result[n] = NULL;

  return result;
}


#if 0
static void
terminal_widget_update_background (TerminalWidget *widget)
{
  if (G_UNLIKELY (widget->background_timer_id != 0))
    g_source_remove (widget->background_timer_id);

  widget->background_timer_id = g_timeout_add_full (G_PRIORITY_LOW, 250, terminal_widget_timer_background,
                                                    widget, terminal_widget_timer_background_destroy);
}
#endif


static void
terminal_widget_update_binding_backspace (TerminalWidget *widget)
{
  vte_terminal_set_delete_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_ASCII_BACKSPACE);
}


static void
terminal_widget_update_binding_delete (TerminalWidget *widget)
{
  vte_terminal_set_delete_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_DELETE_SEQUENCE);
}

static void
terminal_widget_update_misc_bell (TerminalWidget *widget)
{

}


static void
terminal_widget_update_misc_cursor_blinks (TerminalWidget *widget)
{
}

static void
terminal_widget_gconf_scrollback_lines(GConfClient    *client,
                                       guint           conn_id,
				       GConfEntry     *entry,
				       TerminalWidget *widget)
{
  gint lines = OSSO_XTERM_DEFAULT_SCROLLBACK;
  if (entry && entry->value && entry->value->type == GCONF_VALUE_INT) {
    lines = gconf_value_get_int(entry->value);
  }
  if (lines <= 0) lines = OSSO_XTERM_DEFAULT_SCROLLBACK;
  vte_terminal_set_scrollback_lines (VTE_TERMINAL (widget->terminal), lines);
}

static void
terminal_widget_update_scrolling_lines (TerminalWidget *widget)
{
  GConfEntry *entry;
  GConfValue *value;
  value = gconf_client_get (widget->gconf_client,
			    OSSO_XTERM_GCONF_SCROLLBACK, NULL);
  entry = gconf_entry_new_nocopy (g_strdup(OSSO_XTERM_GCONF_SCROLLBACK),
				  value);

  if (widget->scrollback_conid == 0) {
    widget->scrollback_conid = gconf_client_notify_add(
						       widget->gconf_client,
						       OSSO_XTERM_GCONF_SCROLLBACK,
						       (GConfClientNotifyFunc)terminal_widget_gconf_scrollback_lines,
						       widget,
						       NULL, NULL);
  }
  terminal_widget_gconf_scrollback_lines (widget->gconf_client,
					  widget->scrollback_conid,
					  entry,
					  widget);
  gconf_entry_unref(entry);
}


static void
terminal_widget_gconf_scrolling_on_output (GConfClient *client, guint conn_id, GConfEntry *entry, TerminalWidget *widget)
{
  gboolean enable = OSSO_XTERM_DEFAULT_ALWAYS_SCROLL;
  if (entry && entry->value && entry->value->type == GCONF_VALUE_BOOL) {
    enable = gconf_value_get_bool (entry->value);
  }
  vte_terminal_set_scroll_on_output (VTE_TERMINAL (widget->terminal), enable);
}

static void
terminal_widget_update_scrolling_on_output (TerminalWidget *widget)
{
  GConfEntry *entry;
  GConfValue *value;
  value = gconf_client_get (widget->gconf_client,
			    OSSO_XTERM_GCONF_ALWAYS_SCROLL, NULL);
  entry = gconf_entry_new_nocopy (g_strdup(OSSO_XTERM_GCONF_ALWAYS_SCROLL),
				  value);

  if (widget->scrolling_conid == 0) {
    widget->scrolling_conid = gconf_client_notify_add(
						       widget->gconf_client,
						       OSSO_XTERM_GCONF_ALWAYS_SCROLL,
						       (GConfClientNotifyFunc)terminal_widget_gconf_scrolling_on_output,
						       widget,
						       NULL, NULL);
  }
  terminal_widget_gconf_scrolling_on_output (widget->gconf_client,
					  widget->scrolling_conid,
					  entry,
					  widget);
  gconf_entry_unref(entry);
}


static void
terminal_widget_update_scrolling_on_keystroke (TerminalWidget *widget)
{
}


#if 0
static void
terminal_widget_update_title (TerminalWidget *widget)
{
  g_object_notify (G_OBJECT (widget), "title");
}
#endif


static void
terminal_widget_update_word_chars (TerminalWidget *widget)
{
}

static void
terminal_widget_vte_child_exited (VteTerminal    *terminal,
                                  TerminalWidget *widget)
{
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif

  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}


static void
terminal_widget_vte_drag_data_received (VteTerminal      *terminal,
                                        GdkDragContext   *context,
                                        gint              x,
                                        gint              y,
                                        GtkSelectionData *selection_data,
                                        guint             info,
                                        guint             time,
                                        TerminalWidget   *widget)
{
  const guint16 *ucs;
  GString       *str;
  gchar        **uris;
  gchar         *filename;
  gchar         *text;
  gint           n;

  switch (info)
    {
    case TARGET_STRING:
    case TARGET_UTF8_STRING:
    case TARGET_COMPOUND_TEXT:
    case TARGET_TEXT:
      text = (gchar *)gtk_selection_data_get_text (selection_data);
      if (G_LIKELY (text != NULL))
        {
          if (G_LIKELY (*text != '\0'))
            vte_terminal_feed_child (VTE_TERMINAL (terminal), text, strlen (text));
          g_free (text);
        }
      break;

    case TARGET_TEXT_PLAIN:
      if (selection_data->format != 8 || selection_data->length == 0)
        {
          g_printerr (_("Unable to drop selection of type text/plain to terminal: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          vte_terminal_feed_child (VTE_TERMINAL (terminal),
                                   ((const char *)(selection_data->data)),
                                   selection_data->length);
        }
      break;

    case TARGET_MOZ_URL:
      if (selection_data->format != 8
          || selection_data->length == 0
          || (selection_data->length % 2) != 0)
        {
          g_printerr (_("Unable to drop Mozilla URL on terminal: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          str = g_string_new (NULL);
          ucs = (const guint16 *) selection_data->data;
          for (n = 0; n < selection_data->length / 2 && ucs[n] != '\n'; ++n)
            g_string_append_unichar (str, (gunichar) ucs[n]);
          filename = g_filename_from_uri (str->str, NULL, NULL);
          if (filename != NULL)
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget->terminal), filename, strlen (filename));
              g_free (filename);
            }
          else
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget->terminal), str->str, str->len);
            }
          g_string_free (str, TRUE);
        }
      break;

    case TARGET_URI_LIST:
      if (selection_data->format != 8 || selection_data->length == 0)
        {
          g_printerr (_("Unable to drop URI list on terminal: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          text = g_strndup (((const gchar *)(selection_data->data)), selection_data->length);
          uris = g_strsplit (text, "\r\n", 0);
          g_free (text);

          for (n = 0; uris != NULL && uris[n] != NULL; ++n)
            {
              filename = g_filename_from_uri (uris[n], NULL, NULL);
              if (G_LIKELY (filename != NULL))
                {
                  g_free (uris[n]);
                  uris[n] = filename;
                }
            }

          if (uris != NULL)
            {
              text = g_strjoinv (" ", uris);
              vte_terminal_feed_child (VTE_TERMINAL (widget->terminal), text, strlen (text));
              g_strfreev (uris);
            }
        }
      break;
    }
}


static void
terminal_widget_vte_encoding_changed (VteTerminal     *terminal,
                                      TerminalWidget  *widget)
{
  g_object_notify (G_OBJECT (widget), "encoding");
}


static void
terminal_widget_vte_eof (VteTerminal    *terminal,
                         TerminalWidget *widget)
{
  gtk_widget_destroy (GTK_WIDGET (widget));
}


static gboolean
terminal_widget_vte_button_press_event (VteTerminal    *terminal,
                                        GdkEventButton *event,
                                        TerminalWidget *widget)
{
  if (event->button == 3)
    {
      g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
      return TRUE;
    }
  return FALSE;
}

/* Only from N810-keyboard, propably also from bt/usb keyboards */
static gboolean
terminal_widget_vte_key_press_event (VteTerminal    *terminal,
                                     GdkEventKey    *event,
                                     TerminalWidget *widget)
{
  if (event->state == 0 && event->keyval == GDK_Menu)
    {
      g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
      return TRUE;
    }

  return FALSE;
}


static void
terminal_widget_vte_realize (VteTerminal    *terminal,
                             TerminalWidget *widget)
{
  vte_terminal_set_allow_bold (terminal, TRUE);
  terminal_widget_timer_background (TERMINAL_WIDGET (widget));
}


static void
terminal_widget_vte_selection_changed (VteTerminal    *terminal,
                                       TerminalWidget *widget)
{
  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  g_signal_emit (G_OBJECT (widget), widget_signals[SELECTION_CHANGED], 0);
}


static void
terminal_widget_vte_window_title_changed (VteTerminal    *terminal,
                                          TerminalWidget *widget)
{
  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  g_object_notify (G_OBJECT (widget), "title");
}


static gboolean
terminal_widget_timer_background (gpointer user_data)
{
  return FALSE;
}


void
terminal_widget_update_tool_bar (TerminalWidget *widget, gboolean show)
{
  if (show)
    gtk_widget_show (widget->tbar);
  else
    gtk_widget_hide (widget->tbar);
}

static void
terminal_widget_update_keys (TerminalWidget *widget, GSList *keys, GSList *key_labels)
{
  g_slist_foreach(widget->keys_toolbuttons, (GFunc)gtk_widget_destroy, NULL);
  g_slist_foreach(widget->keys_toolbuttons, (GFunc)g_object_unref, NULL);
  g_slist_free(widget->keys_toolbuttons);
  widget->keys_toolbuttons = NULL;

  while (keys && key_labels) {
#ifdef DEBUG
    g_debug ("%s - add %s",__FUNCTION__, (gchar *)key_labels->data);
#endif
    GtkToolItem *button = gtk_tool_button_new(NULL, key_labels->data);
    g_object_set_data_full(G_OBJECT(button), "keys", g_strdup(keys->data), g_free);

    gtk_widget_show(GTK_WIDGET(button));
    gtk_toolbar_insert(GTK_TOOLBAR(widget->tbar), 
		       button, -1);

    g_signal_connect(G_OBJECT(button),
		     "clicked",
		     G_CALLBACK(terminal_widget_do_key_button),
		     widget);
    widget->keys_toolbuttons = g_slist_prepend(
        widget->keys_toolbuttons, g_object_ref(button));

    keys = g_slist_next(keys);
    key_labels = g_slist_next(key_labels);
  }
}

static void
terminal_widget_update_colors (TerminalWidget *widget, const gchar *fg_name, const gchar *bg_name, gboolean reverse)
{
  GdkColor fg, bg;

  gdk_color_parse(fg_name, &fg);
  gdk_color_parse(bg_name, &bg);

  vte_terminal_set_colors(VTE_TERMINAL(widget->terminal),
			  (reverse ? &bg : &fg), (reverse ? &fg : &bg),
			  NULL, 0);
}


static void
terminal_widget_update_font (TerminalWidget *widget, const gchar *name, gint size)
{
  gchar *font_name;
  font_name = g_strdup_printf("%s %d", name, size);
  vte_terminal_set_font_from_string (VTE_TERMINAL (widget->terminal), font_name);
  g_free(font_name);
}


static void
terminal_widget_gconf_toolbar(GConfClient    *client,
			      guint           conn_id,
			      GConfEntry     *entry,
			      TerminalWidget *widget) {
  GConfValue *value;
  gboolean toolbar;

  value = gconf_entry_get_value(entry);
  toolbar = gconf_value_get_bool(value);
  terminal_widget_update_tool_bar(widget, toolbar);
}

static void
terminal_widget_gconf_toolbar_fs(GConfClient    *client,
			      guint           conn_id,
			      GConfEntry     *entry,
			      TerminalWidget *widget) {
  GConfValue *value;
  gboolean toolbar;

  value = gconf_entry_get_value(entry);
  toolbar = gconf_value_get_bool(value);
  terminal_widget_update_tool_bar(widget, toolbar);
}

static void
terminal_widget_gconf_keys(GConfClient    *client,
			   guint           conn_id,
			   GConfEntry     *entry,
			   TerminalWidget *widget)
{
  GSList *keys;
  GSList *key_labels;

  (void)entry;
  (void)conn_id;

  key_labels = gconf_client_get_list(client,
				     OSSO_XTERM_GCONF_KEY_LABELS,
				     GCONF_VALUE_STRING,
				     NULL);
  keys = gconf_client_get_list(client,
			       OSSO_XTERM_GCONF_KEYS,
			       GCONF_VALUE_STRING,
			       NULL);

  terminal_widget_update_keys(widget, keys, key_labels);

  g_slist_foreach(keys, (GFunc)g_free, NULL);
  g_slist_foreach(key_labels, (GFunc)g_free, NULL);
  g_slist_free(keys);
  g_slist_free(key_labels);
}

static void
terminal_widget_gconf_reverse(GConfClient    *client,
                              guint           conn_id,
                              GConfEntry     *entry,
                              TerminalWidget *widget) {
  gchar *fg_name;
  gchar *bg_name;
  gboolean reverse;
  GConfValue *reverse_value;

  fg_name = gconf_client_get_string(client, OSSO_XTERM_GCONF_FONT_COLOR, NULL);
  bg_name = gconf_client_get_string(client, OSSO_XTERM_GCONF_BG_COLOR, NULL);
  reverse_value = gconf_client_get(client, OSSO_XTERM_GCONF_REVERSE, NULL);
  if (reverse_value) {
    if (reverse_value->type == GCONF_VALUE_BOOL) {
      reverse = gconf_value_get_bool(reverse_value);
    } else {
      reverse = OSSO_XTERM_DEFAULT_REVERSE;
    }
    gconf_value_free(reverse_value);
  } else {
    reverse = OSSO_XTERM_DEFAULT_REVERSE;
  }

  if (!fg_name) {
    fg_name = g_strdup(OSSO_XTERM_DEFAULT_FONT_COLOR);
  }
  if (!bg_name) {
    bg_name = g_strdup(OSSO_XTERM_DEFAULT_BG_COLOR);
  }

  terminal_widget_update_colors(widget, fg_name, bg_name, reverse);

  g_free(fg_name);
  g_free(bg_name);
}


static void
terminal_widget_gconf_font_size(GConfClient    *client,
                                guint           conn_id,
                                GConfEntry     *entry,
                                TerminalWidget *widget) {
  gchar *font_name;
  gint font_size;

  (void)conn_id;
  (void)entry;

  font_name = gconf_client_get_string(client, OSSO_XTERM_GCONF_FONT_NAME, NULL);
  if (!font_name) {
    font_name = g_strdup(OSSO_XTERM_DEFAULT_FONT_NAME);
  }
  font_size = gconf_client_get_int(client, OSSO_XTERM_GCONF_FONT_BASE_SIZE, NULL);
  if (!font_size) {
    font_size = OSSO_XTERM_DEFAULT_FONT_BASE_SIZE;
  }
  font_size += gconf_client_get_int(client, OSSO_XTERM_GCONF_FONT_SIZE, NULL);

  terminal_widget_update_font(widget, font_name, font_size);
  g_free(font_name);
}

#if 0
static void
terminal_widget_timer_background_destroy (gpointer user_data)
{
  TERMINAL_WIDGET (user_data)->background_timer_id = 0;
}
#endif


/**
 * terminal_widget_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_widget_new ()
{
  return g_object_new(TERMINAL_TYPE_WIDGET, NULL);
}

static gboolean
terminal_widget_window_delete_event(
    HildonWindow *window, GdkEvent *event, TerminalWidget *widget)
{
  hildon_window_remove_toolbar (HILDON_WINDOW (widget->app),
      GTK_TOOLBAR (widget->tbar));
  g_signal_handlers_disconnect_by_func(widget->app,
        terminal_widget_window_delete_event, widget);
  g_object_unref(widget->app);

  return FALSE;
}

void
terminal_widget_set_app_win (TerminalWidget *widget, HildonWindow *window)
{
  widget->app = g_object_ref(window);
  g_signal_connect (G_OBJECT (widget->app), "delete-event",
                    G_CALLBACK (terminal_widget_window_delete_event), widget);

#ifdef DEBUG
  g_debug("%s - tbar: %p", __FUNCTION__, widget->tbar);
#endif

  hildon_window_add_toolbar (HILDON_WINDOW (widget->app), GTK_TOOLBAR (widget->tbar));
}

/**
 * terminal_widget_launch_child:
 * @widget  : A #TerminalWidget.
 *
 * Starts the terminal child process.
 **/
gboolean
terminal_widget_launch_child (TerminalWidget *widget)
{
  GError  *error = NULL;
  gchar   *command;
  gchar  **argv;
  gchar  **env;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), FALSE);

  if (!terminal_widget_get_child_command (widget, &command, &argv, &error))
    {
      g_warning("Can't create new terminal widget because: %s", error->message);
      g_error_free (error);
      return FALSE;
    }

  env = terminal_widget_get_child_environment (widget);

  widget->pid = vte_terminal_fork_command (VTE_TERMINAL (widget->terminal),
                                           command, argv, env,
                                           widget->working_directory,
                                           TRUE, TRUE, TRUE);

  g_strfreev (argv);
  g_strfreev (env);
  g_free (command);

  return (widget->pid > 0);
}


/**
 * terminal_widget_set_custom_command:
 * @widget  : A #TerminalWidget.
 * @command :
 **/
void
terminal_widget_set_custom_command (TerminalWidget *widget,
                                    gchar         **command)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (G_UNLIKELY (widget->custom_command != NULL))
    g_strfreev (widget->custom_command);

  if (G_LIKELY (command != NULL && *command != NULL))
    widget->custom_command = g_strdupv (command);
  else
    widget->custom_command = NULL;
}


/**
 * terminal_widget_set_custom_title:
 * @widget  : A #TerminalWidget.
 * @title   : Title string.
 **/
void
terminal_widget_set_custom_title (TerminalWidget *widget,
                                  const gchar    *title)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (g_ascii_strcasecmp(widget->custom_title, title))
    {
      g_free (widget->custom_title);
      widget->custom_title = g_strdup (title != NULL ? title : "");
      g_object_notify (G_OBJECT (widget), "custom-title");
      g_object_notify (G_OBJECT (widget), "title");
    }
}


/**
 **/
void
terminal_widget_get_size (TerminalWidget *widget,
                          gint           *width_chars,
                          gint           *height_chars)
{
  *width_chars = VTE_TERMINAL (widget->terminal)->column_count;
  *height_chars = VTE_TERMINAL (widget->terminal)->row_count;
}

/**
 * terminal_widget_force_resize_window:
 *
 * I don't like this way, but its required to work-around a Gtk+
 * bug (maybe also caused by a Vte bug, not sure).
 **/
void
terminal_widget_force_resize_window (TerminalWidget *widget,
                                     GtkWindow      *window,
                                     gint            force_columns,
                                     gint            force_rows)
{
  GtkRequisition terminal_requisition;
  GtkRequisition window_requisition;
  gint           width;
  gint           height;
  gint           columns;
  gint           rows;
  gint           xpad;
  gint           ypad;

  gtk_widget_set_size_request (widget->terminal, 2000, 2000);
  gtk_widget_size_request (GTK_WIDGET (window), &window_requisition);
  gtk_widget_size_request (widget->terminal, &terminal_requisition);

  width = window_requisition.width - terminal_requisition.width;
  height = window_requisition.height - terminal_requisition.height;

  if (force_columns < 0)
    columns = VTE_TERMINAL (widget->terminal)->column_count;
  else
    columns = force_columns;

  if (force_rows < 0)
    rows = VTE_TERMINAL (widget->terminal)->row_count;
  else
    rows = force_rows;

  vte_terminal_get_padding (VTE_TERMINAL (widget->terminal), &xpad, &ypad);

  width += xpad + VTE_TERMINAL (widget->terminal)->char_width * columns;
  height += ypad + VTE_TERMINAL (widget->terminal)->char_height * rows;

  if (width < 0 || height < 0) {
    g_printerr("Invalid values: width=%d, height=%d, rows=%d, cols=%d\n",
	       width, height, rows, columns);
    return;
  }

  if (GTK_WIDGET_MAPPED (window))
    gtk_window_resize (window, width, height);
  else
    gtk_window_set_default_size (window, width, height);
}


/**
 * terminal_widget_set_window_geometry_hints:
 *
 * I don't like this way, but its required to work-around a Gtk+
 * bug (maybe also caused by a Vte bug, not sure).
 **/
void
terminal_widget_set_window_geometry_hints (TerminalWidget *widget,
                                           GtkWindow      *window)
{
  GdkGeometry hints;
  gint        xpad;
  gint        ypad;

  vte_terminal_get_padding (VTE_TERMINAL (widget->terminal), &xpad, &ypad);

  hints.base_width = xpad;
  hints.base_height = ypad;
  hints.width_inc = VTE_TERMINAL (widget->terminal)->char_width;
  hints.height_inc = VTE_TERMINAL (widget->terminal)->char_height;
  hints.min_width = hints.base_width + hints.width_inc * 4;
  hints.min_height = hints.base_height + hints.height_inc * 2;

  gtk_window_set_geometry_hints (GTK_WINDOW (window),
                                 widget->terminal,
                                 &hints,
                                 GDK_HINT_RESIZE_INC
                                 | GDK_HINT_MIN_SIZE
                                 | GDK_HINT_BASE_SIZE);
}


/**
 * terminal_widget_get_title:
 * @widget      : A #TerminalWidget.
 *
 * Return value :
 **/
gchar*
terminal_widget_get_title (TerminalWidget *widget)
{
  const gchar  *window_title;
  gchar *title;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  if (G_UNLIKELY (*widget->custom_title != '\0'))
    return g_strdup (widget->custom_title);

  window_title = vte_terminal_get_window_title (VTE_TERMINAL (widget->terminal));

  if (window_title != NULL)
    title = g_strdup (window_title);
  else
    title = g_strdup (_("Untitled"));

  return title;
}


/**
 * terminal_widget_get_working_directory:
 * @widget      : A #TerminalWidget.
 *
 * Determinies the working directory using various OS-specific mechanisms.
 *
 * Return value : The current working directory of @widget.
 **/
const gchar*
terminal_widget_get_working_directory (TerminalWidget *widget)
{
  gchar  buffer[4096 + 1];
  gchar *file;
  gchar *cwd;
  gint   length;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  if (widget->pid >= 0)
    {
      file = g_strdup_printf ("/proc/%d/cwd", widget->pid);
      length = readlink (file, buffer, sizeof (buffer));

      if (length > 0 && *buffer == '/')
        {
          buffer[length] = '\0';
          g_free (widget->working_directory);
          widget->working_directory = g_strdup (buffer);
        }
      else if (length == 0)
        {
          cwd = g_get_current_dir ();
          if (G_LIKELY (cwd != NULL))
            {
              if (chdir (file) == 0)
                {
                  g_free (widget->working_directory);
                  widget->working_directory = g_get_current_dir ();
                  chdir (cwd);
                }

              g_free (cwd);
            }
        }

      g_free (file);
    }

  return widget->working_directory;
}


/**
 * terminal_widget_set_working_directory:
 * @widget    : A #TerminalWidget.
 * @directory :
 **/
void
terminal_widget_set_working_directory (TerminalWidget *widget,
                                       const gchar    *directory)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  g_return_if_fail (directory != NULL);

  g_free (widget->working_directory);
  widget->working_directory = g_strdup (directory);
}


/**
 * terminal_widget_has_selection:
 * @widget      : A #TerminalWidget.
 *
 * Checks if the terminal currently contains selected text. Note that this is different from
 * determining if the terminal is the owner of any GtkClipboard items.
 *
 * Return value : %TRUE if part of the text in the terminal is selected.
 **/
gboolean
terminal_widget_has_selection (TerminalWidget *widget)
{
  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), FALSE);
  return vte_terminal_get_has_selection (VTE_TERMINAL (widget->terminal));
}


/**
 * terminal_widget_copy_clipboard:
 * @widget  : A #TerminalWidget.
 *
 * Places the selected text in the terminal in the #GDK_SELECTIN_CLIPBOARD selection.
 **/
void
terminal_widget_copy_clipboard (TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  vte_terminal_copy_clipboard (VTE_TERMINAL (widget->terminal));
}


/**
 * terminal_widget_paste_clipboard:
 * @widget  : A #TerminalWidget.
 *
 * Sends the contents of the #GDK_SELECTION_CLIPBOARD selection to the terminal's
 * child. If neccessary, the data is converted from UTF-8 to the terminal's current
 * encoding.
 **/
void
terminal_widget_paste_clipboard (TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  vte_terminal_paste_clipboard (VTE_TERMINAL (widget->terminal));
}


/**
 * terminal_widget_reset:
 * @widget  : A #TerminalWidget.
 * @clear   : %TRUE to also clear the terminal screen.
 *
 * Resets the terminal.
 **/
void
terminal_widget_reset (TerminalWidget *widget,
                       gboolean        clear)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  vte_terminal_reset (VTE_TERMINAL (widget->terminal), TRUE, clear);
}

/**
 * terminal_widget_im_append_menuitems:
 * @widget    : A #TerminalWidget.
 * @menushell : A #GtkMenuShell.
 *
 * Appends menu items for various input methods to the given @menushell.
 * The user can select one of these items to modify the input method
 * used by the terminal.
 **/
void
terminal_widget_im_append_menuitems (TerminalWidget *widget,
                                     GtkMenuShell   *menushell)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  g_return_if_fail (GTK_IS_MENU_SHELL (menushell));

  vte_terminal_im_append_menuitems (VTE_TERMINAL (widget->terminal), menushell);
}


static void
terminal_widget_emit_context_menu (TerminalWidget *widget,
		                   gpointer        user_data)
{
  GdkEvent *event = gdk_event_new(GDK_BUTTON_PRESS);
  GdkEventButton *button = (GdkEventButton *)event;
  gint x, y;

  (void)user_data;

  gtk_widget_get_pointer(widget->terminal, &x, &y);
  button->button = GDK_BUTTON_PRESS;
  button->window = widget->terminal->window;
  button->send_event = FALSE;
  button->time = gtk_get_current_event_time();
  button->x = x;
  button->y = y;
  button->axes = NULL;
  button->state = 0;
  button->button = 0;
  button->device = NULL;

  g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
  /*  g_free(button); */
}

char *
terminal_widget_get_tag (TerminalWidget *widget,
		         gint            x,
			 gint            y,
			 gint           *tag)
{
  VteTerminal *term = VTE_TERMINAL(widget->terminal);
  int xpad, ypad;

  vte_terminal_get_padding(term,
			   &xpad, &ypad);

  return vte_terminal_match_check(term,
				  (x - xpad) / term->char_width,
				  (y - ypad) / term->char_height,
				  tag);
}

static void
terminal_widget_ctrlify_notify (GObject *src,
				    GParamSpec     *pspec,
				    GObject *dst)
{
  const char *dst_pspec_name = strcmp(pspec->name, "active") ? "active" : "control-mask";
  gboolean src_val, dst_val;

  g_object_get(src, pspec->name, &src_val, NULL);
  g_object_get(dst, dst_pspec_name, &dst_val, NULL);
  if (src_val != dst_val)
    g_object_set(dst, dst_pspec_name, src_val, NULL);
}

static void
terminal_widget_send_key(TerminalWidget *widget,
		         guint keyval,
			 guint state)
{
  GdkEventKey *key;
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  key = (GdkEventKey *) gdk_event_new(GDK_KEY_PRESS);

  key->window = GTK_WIDGET(widget->terminal)->window;
  key->time = GDK_CURRENT_TIME;
  key->state = state;
  key->keyval = keyval;
  gdk_event_put ((GdkEvent *) key);

  key->type = GDK_KEY_RELEASE;
  key->state |= GDK_RELEASE_MASK;
  gdk_event_put ((GdkEvent *) key);

  g_object_ref(key->window);
  gdk_event_free((GdkEvent *) key);
#ifdef DEBUG
  g_debug ("%s - end", __FUNCTION__);
#endif
}

static const struct {
  const gchar *name;
  GdkModifierType mask;
} modifier_table[11] = {
  { "shift", GDK_SHIFT_MASK },
  { "lock", GDK_LOCK_MASK },
  { "ctrl", GDK_CONTROL_MASK },
  { "control", GDK_CONTROL_MASK },
  { "mod1", GDK_MOD1_MASK },
  { "alt", GDK_MOD1_MASK },
  { "mod2", GDK_MOD2_MASK },
  { "mod3", GDK_MOD3_MASK },
  { "mod4", GDK_MOD4_MASK },
  { "mod5", GDK_MOD5_MASK },
  { NULL, 0 }
};

static GdkModifierType get_mask(const gchar *name, const gchar *end) {
  int i;

  for (i = 0; modifier_table[i].name; i++) {
    if (!strncmp(name, modifier_table[i].name, end - name)) {
      break;
    }
  }
  return modifier_table[i].mask;
}

static const gchar *parse_key(const gchar *source,
			      guint *keyval,
			      guint *state)
{
  const gchar *tag_start = NULL;
  const gchar *key_start = NULL;
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
  if (!source || !keyval || !state) {
    return NULL;
  }

  *keyval = 0;
  *state = 0;

  while (*source) {
    switch (*source) {
    case '<':
      if (!tag_start) {
	tag_start = source + 1;
      }
      break;
    case '>':
      if (tag_start) {
	*state |= get_mask(tag_start, source - 1);
	tag_start = NULL;
      } else {
	key_start = source;
      }
      break;
    case '\\':
      if (!key_start) {
	key_start = source + 1;
      }
      break;
    case ',':
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      if (key_start && !tag_start) {
	gchar *temp = g_strndup(key_start,
				source - key_start);
	*keyval = gdk_keyval_from_name(temp);
	g_free(temp);
	return source + 1;
      }
      break;
    default:
      if (!tag_start && !key_start) {
	key_start = source;
      }
      break;
    }
    source++;
  }
  if (key_start) {
    *keyval = gdk_keyval_from_name(key_start);
    return source;
  }
  return NULL;
}

static void 
terminal_widget_do_keys(TerminalWidget *widget,
			const gchar *key_string)
{
  guint keyval = 0;
  guint state = 0;

  while (key_string && *key_string) {
    key_string = parse_key(key_string, &keyval, &state);
    terminal_widget_send_key(widget, keyval, state);
  }
}

static void 
terminal_widget_do_key_button(GObject *button,
			      TerminalWidget *widget)
{
  terminal_widget_do_keys(widget, g_object_get_data(button, "keys"));
}

gboolean   
terminal_widget_select_all (TerminalWidget *widget)
{
#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif
#if 0
  VteTerminal *term = VTE_TERMINAL(widget->terminal);
  GTK_WIDGET (term->widget);
#endif
  return TRUE;
}
#if 0
void terminal_widget_send_keys(TerminalWidget *widget,
                               const gchar *key_string)
{
        guint keyval = 0;
        guint state = 0;

        while (key_string && *key_string) {
                key_string = parse_key(key_string, &keyval, &state);
                terminal_widget_send_key(widget, keyval, state);
        }
}
#endif

void
terminal_widget_add_tool_item(TerminalWidget *widget, GtkToolItem *item)
{
  gtk_toolbar_insert(GTK_TOOLBAR(widget->tbar), item, -1);
}

gboolean
terminal_widget_modify_font_size(TerminalWidget *widget, int increment)
{
  int font_size_delta = gconf_client_get_int(widget->gconf_client, OSSO_XTERM_GCONF_FONT_SIZE, NULL);
  font_size_delta += increment;

  if (ABS(font_size_delta) <= FONT_SIZE_MAX_ABS_DELTA)
    return gconf_client_set_int(widget->gconf_client, OSSO_XTERM_GCONF_FONT_SIZE, font_size_delta, NULL);

  return FALSE;
}
