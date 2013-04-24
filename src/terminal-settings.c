/*
* Copyright (C) 2005 Nokia Corporation
*
* Written by Johan Hedberg <johan.hederg@nokia.com>
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
*/

#define GETTEXT_PACKAGE "osso-browser-ui"

#include <gconf/gconf-client.h>

#include <glib/gi18n-lib.h>

#include <libintl.h>
#include <locale.h>

#include <stdlib.h>
#if HILDON == 0
#include <hildon-widgets/hildon-color-button.h>
#include <hildon-widgets/hildon-font-selection-dialog.h>
#elif HILDON == 1
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-font-selection-dialog.h>
#endif

#include "terminal-gconf.h"
#include "terminal-settings.h"
#include "terminal-encoding.h"
#include "shortcuts.h"

#define MAX_FONT_NAME_LENGHT 256
#define DEFAULT_BUTTON_HEIGHT 50

static void terminal_settings_class_init    (TerminalSettingsClass *klass);
static void terminal_settings_init          (TerminalSettings      *header);
static void terminal_settings_finalize      (GObject               *object);
static void terminal_settings_show_hildon_font_dialog (GtkButton *button, gpointer data);
static void terminal_widget_edit_shortcuts (GtkButton *button, gpointer *data);
static void terminal_settings_set_default_charset (GtkButton *button, 
						   gpointer data);

struct _TerminalSettings
{
  GtkDialog    __parent__;

  GtkWidget   *font_button;
  GtkWidget   *fg_button;
  GdkColor    *color;
  GtkWidget   *bg_button;
  GtkWidget   *sb_spinner;

  gchar *encoding;
};


static GObjectClass *parent_class;

G_DEFINE_TYPE (TerminalSettings, terminal_settings, GTK_TYPE_DIALOG);


static void
terminal_settings_class_init (TerminalSettingsClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_settings_finalize;
}


static void
terminal_settings_init (TerminalSettings *settings)
{
  GtkWidget *widget = NULL;
  GConfClient *gc = gconf_client_get_default();
  GdkColor fg;
  GdkColor bg;
  gint sb;
  gchar *color_name;
  gchar *font_name = gconf_client_get_string(gc, OSSO_XTERM_GCONF_FONT_NAME, NULL);
  gchar *font = NULL;

  settings->encoding = NULL;
  settings->color = NULL;

  if (!font_name) {
    font_name = g_strdup(OSSO_XTERM_DEFAULT_FONT_NAME);
  }

  gint font_size = gconf_client_get_int(gc, OSSO_XTERM_GCONF_FONT_BASE_SIZE, NULL);
  if (!font_size) {
    font_size = OSSO_XTERM_DEFAULT_FONT_BASE_SIZE;
  }

  color_name = gconf_client_get_string(gc, OSSO_XTERM_GCONF_FONT_COLOR, NULL);
  if (!color_name) {
    gdk_color_parse(OSSO_XTERM_DEFAULT_FONT_COLOR, &fg);
  } else {
    if(gdk_color_parse(color_name, &fg) == FALSE) { 
#ifdef DEBUG
      g_debug ("color parsing failed");
#endif
      gdk_color_parse(OSSO_XTERM_DEFAULT_FONT_COLOR, &fg);
    }
    g_free(color_name);
  }

  settings->color = gdk_color_copy (&fg);

  color_name = gconf_client_get_string(gc, OSSO_XTERM_GCONF_BG_COLOR, NULL);
  if (!color_name) {
    gdk_color_parse(OSSO_XTERM_DEFAULT_BG_COLOR, &bg);
  } else {
    gdk_color_parse(color_name, &bg);
    g_free(color_name);
  }

  sb = gconf_client_get_int(gc, OSSO_XTERM_GCONF_SCROLLBACK, NULL);
  if (!sb) {
    sb = OSSO_XTERM_DEFAULT_SCROLLBACK;
  }
  if (sb <= 0) sb = OSSO_XTERM_DEFAULT_SCROLLBACK;

  g_object_unref(gc);

  font = g_strdup_printf("%s %d", font_name, font_size);
  g_free(font_name);
  font_name = NULL;

  settings->font_button = gtk_font_button_new_with_font(font);
  gtk_widget_set_size_request (settings->font_button, 
			       -1, DEFAULT_BUTTON_HEIGHT);
  gtk_widget_show (settings->font_button);

  /* FIXME: way to do this is subclassing */
  GTK_BUTTON_GET_CLASS (settings->font_button)->clicked = NULL;
  gtk_signal_connect( GTK_OBJECT (settings->font_button), "clicked", 
		      G_CALLBACK (terminal_settings_show_hildon_font_dialog), 
		      settings);

  g_free(font);
  font = NULL;

  settings->bg_button = hildon_color_button_new_with_color(&bg);
  gtk_widget_set_size_request (settings->bg_button, 
			       -1, DEFAULT_BUTTON_HEIGHT);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG(settings)->vbox), 6);
  gtk_widget_show (settings->bg_button);

  gchar labeltext[256];
  GtkWidget *hbox = gtk_hbox_new (TRUE, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_widget_show (hbox);

  /* FIXME: g_...dup should be used */
  g_snprintf (labeltext, 255, "%s:", dgettext ("hildon-libs", "ecdg_ti_font"));

  GtkWidget *align = gtk_alignment_new (1, 0.5, 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (align), 6);
  gtk_widget_show (align);

  widget = gtk_label_new (labeltext);
  gtk_widget_show (widget);

  gtk_container_add(GTK_CONTAINER(align), widget);
  gtk_container_add(GTK_CONTAINER(hbox), align);
  gtk_container_add(GTK_CONTAINER(hbox), settings->font_button);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), hbox);

  hbox = gtk_hbox_new (TRUE, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_widget_show (hbox);

  g_snprintf (labeltext, 255, "%s:", dgettext ("maemo-af-desktop", "home_fi_set_backgr_color"));
  align = gtk_alignment_new (1, 0.5, 0, 0);
  gtk_widget_show (align);
  gtk_container_set_border_width (GTK_CONTAINER (align), 6);

  widget = gtk_label_new (labeltext);
  gtk_widget_show (widget);
  gtk_container_add(GTK_CONTAINER(align), widget);
  gtk_container_add(GTK_CONTAINER(hbox), align);

  gtk_container_add(GTK_CONTAINER(hbox), settings->bg_button);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), hbox);


  GtkWidget *button = gtk_button_new_with_label (_("weba_me_encoding"));
  gtk_widget_set_size_request (button, -1, DEFAULT_BUTTON_HEIGHT);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (terminal_settings_set_default_charset), 
		    (gpointer)settings);
  gtk_widget_show (button);

  align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_widget_show (align);
  gtk_container_set_border_width (GTK_CONTAINER (align), 6);

  gtk_container_add (GTK_CONTAINER(align), button);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (settings)->vbox), align);

  button = gtk_button_new_with_label (_("weba_fi_plugin_details_shortcut"));
  gtk_widget_set_size_request (button, -1, DEFAULT_BUTTON_HEIGHT);
  align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_widget_show (align);
  gtk_container_set_border_width (GTK_CONTAINER (align), 6);

  g_signal_connect (button, "clicked", G_CALLBACK (terminal_widget_edit_shortcuts), (gpointer)settings);
  gtk_widget_show (button);

  gtk_container_add(GTK_CONTAINER(align), button);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), align);

  gtk_dialog_add_buttons(GTK_DIALOG(settings),
			 _("weba_bd_ok"), GTK_RESPONSE_OK,
			 _("weba_bd_cancel"), GTK_RESPONSE_CANCEL,
			 NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (settings), FALSE);

}


static void
terminal_settings_finalize (GObject *object)
{

  parent_class->finalize (object);
}


gboolean
terminal_settings_store (TerminalSettings *settings, TerminalWidget *terminal)
{
  GConfClient *gc = gconf_client_get_default();
  const gchar *font = gtk_font_button_get_font_name(GTK_FONT_BUTTON(settings->font_button));
  const gchar *sep = g_utf8_strrchr(font, -1, ' ');
  gchar *color_name;
  GdkColor *color;
#if HILDON == 1
  GdkColor colors;
#endif

  if (!sep) return FALSE;

  gchar *font_name = g_strndup(font, (sep - font));

  gconf_client_set_string(gc, OSSO_XTERM_GCONF_FONT_NAME, font_name, NULL);
  gconf_client_set_int(gc, OSSO_XTERM_GCONF_FONT_BASE_SIZE, atoi(sep + 1), NULL);

  g_free(font_name);

  color = gdk_color_copy(settings->color);
  color_name = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
#ifdef DEBUG
  g_debug ("color : %s", color_name);
#endif
  gconf_client_set_string(gc, OSSO_XTERM_GCONF_FONT_COLOR, color_name, NULL);
  g_free(color_name);

  gdk_color_free (color);
  color = &colors;

#if HILDON == 0
  color = hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->bg_button));
#elif HILDON == 1
  hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->bg_button), color);
#endif
  color_name = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
  gconf_client_set_string(gc, OSSO_XTERM_GCONF_BG_COLOR, color_name, NULL);
  g_free(color_name);

  if (settings->encoding != NULL) {
    gconf_client_set_string(gc, OSSO_XTERM_GCONF_ENCODING, settings->encoding, 
			    NULL);
    g_object_set (terminal, "encoding", settings->encoding, NULL);
  }
  g_object_unref(gc);

  return TRUE;
}


/**
 * terminal_settings_new:
 * 
 * Allocates a new #TerminalSettings object.
 *
 * Return value : Pointer to the allocated #TerminalSettings object.
 **/
GtkWidget*
terminal_settings_new (void)
{
  GtkWidget *dialog;
 
  dialog = g_object_new (TERMINAL_TYPE_SETTINGS, NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), _("weba_ti_settings_title"));

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

  return dialog;
}

/* Show hildon font selector dialog */
static void
terminal_settings_show_hildon_font_dialog (GtkButton *button, gpointer data)
{
  TerminalSettings *settings = TERMINAL_SETTINGS (data);
  PangoFontDescription *fontdesc;

  gboolean bold = FALSE;
  gboolean italic = FALSE;

#ifdef DEBUG
  g_debug (__FUNCTION__);
#endif

  /* Create dialog */
  GtkWidget *dialog = hildon_font_selection_dialog_new (NULL, NULL);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (settings));
  if (dialog == NULL) {
#ifdef DEBUG
    g_debug ("Couldn't create Dialog");
#endif
    return;
  }

  fontdesc = pango_font_description_from_string (
						 gtk_font_button_get_font_name(GTK_FONT_BUTTON (settings->font_button)));
  if (fontdesc == NULL) {
#ifdef DEBUG
    g_debug ("Couldn't create font description");
#endif
    gtk_widget_destroy (dialog);
    return;
  }

  pango_font_description_unset_fields (fontdesc, 
				       PANGO_FONT_MASK_STRETCH|PANGO_FONT_MASK_VARIANT);

  if (pango_font_description_get_style (fontdesc) & PANGO_STYLE_ITALIC) {
    italic = TRUE;
  }

  if ( (pango_font_description_get_weight (fontdesc) & PANGO_WEIGHT_BOLD) 
       == PANGO_WEIGHT_BOLD) {
    bold = TRUE;
  }

  g_object_set(G_OBJECT(dialog),
	       "family", pango_font_description_get_family (fontdesc),
	       "size", pango_font_description_get_size (fontdesc)/PANGO_SCALE,
	       "bold", bold,
	       "italic", italic,
	       "color", settings->color,
	       NULL);

  gtk_widget_show_all (GTK_WIDGET(dialog));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {

    gchar *font_name;
    gint size;
    gchar *family = NULL;

    PangoStyle style = PANGO_STYLE_NORMAL;
    PangoWeight weight = PANGO_WEIGHT_NORMAL;

    g_object_get(G_OBJECT(dialog),
		 "family", &family,
		 "size", &size,
		 "bold", &bold,
		 "italic", &italic,
		 "color", &settings->color,
		 NULL);

    style = PANGO_STYLE_NORMAL;
    weight = PANGO_WEIGHT_NORMAL;

    if (italic == TRUE) {
      style = PANGO_STYLE_ITALIC;
    }
    if (bold == TRUE) {
      weight = PANGO_WEIGHT_BOLD;
    }

    pango_font_description_set_family (fontdesc, family);
    pango_font_description_set_size (fontdesc, size*PANGO_SCALE);
    pango_font_description_set_weight (fontdesc, weight);
    pango_font_description_set_style (fontdesc, style);

    font_name = pango_font_description_to_string (fontdesc);

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (settings->font_button),
				   font_name);
    if (font_name != NULL) {
      g_free(font_name);
    }
    if (family != NULL) {
      g_free (family);
      family = NULL;
    }

  }
  gtk_widget_destroy(dialog);
        
}


static void
terminal_widget_edit_shortcuts (GtkButton *button,
				gpointer  *data)
{
  (void)button;

  update_shortcut_keys(data);
}

static void
terminal_settings_set_default_charset (GtkButton *button, gpointer data)
{
  TerminalSettings *settings = TERMINAL_SETTINGS (data);
  GConfClient *gc = gconf_client_get_default();
  gchar *retval = NULL;
  GError *error = NULL;

  if (settings->encoding == NULL) {
    settings->encoding = gconf_client_get_string (gc, 
						  OSSO_XTERM_GCONF_ENCODING,
						  &error);
  }

  retval = terminal_encoding_dialog (NULL, GTK_WINDOW (settings), 
				     settings->encoding);
  settings->encoding = retval;

}
