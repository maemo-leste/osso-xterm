/*-
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
#include "shortcuts.h"

#define MAX_FONT_NAME_LENGHT 256

static void terminal_settings_class_init    (TerminalSettingsClass *klass);
static void terminal_settings_init          (TerminalSettings      *header);
static void terminal_settings_finalize      (GObject               *object);
static void terminal_settings_show_hildon_font_dialog (GtkButton *button, gpointer data);
static void terminal_widget_edit_shortcuts (GtkButton *button, gpointer *data);

struct _TerminalSettings
{
  GtkDialog    __parent__;

  GtkWidget   *font_button;
  GtkWidget   *fg_button;
  GdkColor    *color;
  GtkWidget   *bg_button;
  GtkWidget   *sb_spinner;

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
      //        g_debug ("color: %s", color_name);
	    if(gdk_color_parse(color_name, &fg) == FALSE) {
            g_debug ("color parsing failed");
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

    g_object_unref(gc);

    font = g_strdup_printf("%s %d", font_name, font_size);
    g_free(font_name);
    font_name = NULL;

    settings->font_button = gtk_font_button_new_with_font(font);

    /* FIXME: right way to do this is subclassing */
    GTK_BUTTON_GET_CLASS (settings->font_button)->clicked = NULL;
    gtk_signal_connect( GTK_OBJECT (settings->font_button), "clicked", 
 			G_CALLBACK (terminal_settings_show_hildon_font_dialog), 
                        settings);

    g_free(font);
    font = NULL;

//    settings->fg_button = hildon_color_button_new_with_color(&fg);
    settings->bg_button = hildon_color_button_new_with_color(&bg);
//    settings->sb_spinner = gtk_spin_button_new_with_range(1.0, 10000.0, 1.0);

//    gtk_spin_button_set_value(GTK_SPIN_BUTTON(settings->sb_spinner), (gdouble)sb);
//    hildon_gtk_entry_set_input_mode(GTK_ENTRY(settings->sb_spinner), HILDON_GTK_INPUT_MODE_NUMERIC);

    gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG(settings)->vbox), 6);

    gchar labeltext[256];
    GtkWidget *hbox = gtk_hbox_new (TRUE, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

    g_snprintf (labeltext, 255, "%s:", dgettext ("osso-email-old", "mcen_ti_font_settings"));
    GtkWidget *align = gtk_alignment_new (1, 0.5, 0, 0);
    gtk_container_set_border_width (GTK_CONTAINER (align), 6);

	widget = gtk_label_new (labeltext);
    gtk_container_add(GTK_CONTAINER(align), widget);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_container_add(GTK_CONTAINER(hbox), settings->font_button);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), hbox);

    hbox = gtk_hbox_new (TRUE, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

    g_snprintf (labeltext, 255, "%s:", dgettext ("osso-email-old", "mcen_me_editor_bgcolor"));
    align = gtk_alignment_new (1, 0.5, 0, 0);
    gtk_container_set_border_width (GTK_CONTAINER (align), 6);

	widget = gtk_label_new (labeltext);
    gtk_container_add(GTK_CONTAINER(align), widget);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_container_add(GTK_CONTAINER(hbox), settings->bg_button);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), hbox);

    GtkWidget *button = gtk_button_new_with_label (_("weba_fi_plugin_details_shortcut"));
    align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_container_set_border_width (GTK_CONTAINER (align), 6);

    g_signal_connect (button, "clicked", G_CALLBACK (terminal_widget_edit_shortcuts), (gpointer)settings);
    gtk_widget_show (button);

    gtk_container_add(GTK_CONTAINER(align), button);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), align);

    gtk_dialog_add_buttons(GTK_DIALOG(settings),
                           _("weba_bd_ok"), GTK_RESPONSE_OK,
                           _("weba_bd_cancel"), GTK_RESPONSE_CANCEL,
                           NULL);

    gtk_widget_show_all(GTK_WIDGET(settings));
    gtk_widget_set_size_request (GTK_WIDGET(settings), 400, 300);
}


static void
terminal_settings_finalize (GObject *object)
{
//    TerminalSettings *settings = TERMINAL_SETTINGS (object);
//    if (settings->color != NULL)
//        gdk_color_free (settings->color);

    parent_class->finalize (object);
}


gboolean
terminal_settings_store (TerminalSettings *settings)
{
    GConfClient *gc = gconf_client_get_default();
    const gchar *font = gtk_font_button_get_font_name(GTK_FONT_BUTTON(settings->font_button));
    const gchar *sep = g_utf8_strrchr(font, -1, ' ');
    gchar *color_name;
//    gint sb;
    GdkColor *color;
#if HILDON == 1
//    GdkColor colors;
//    color = &colors;
#endif

    if (!sep) return FALSE;

    gchar *font_name = g_strndup(font, (sep - font));

    gconf_client_set_string(gc, OSSO_XTERM_GCONF_FONT_NAME, font_name, NULL);
    gconf_client_set_int(gc, OSSO_XTERM_GCONF_FONT_BASE_SIZE, atoi(sep + 1), NULL);

    g_free(font_name);

#if 0
#if HILDON == 0
    color = hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->fg_button));
#elif HILDON == 1
    hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->fg_button), color);
#endif
#else
    color = gdk_color_copy(settings->color);
#endif
    color_name = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
    g_debug ("color : %s", color_name);
    gconf_client_set_string(gc, OSSO_XTERM_GCONF_FONT_COLOR, color_name, NULL);
    g_free(color_name);
    gdk_color_free (color);

#if HILDON == 0
    color = hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->bg_button));
#elif HILDON == 1
    hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->bg_button), color);
#endif
    color_name = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
    gconf_client_set_string(gc, OSSO_XTERM_GCONF_BG_COLOR, color_name, NULL);
    g_free(color_name);

  //  sb = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(settings->sb_spinner));
//    gconf_client_set_int(gc, OSSO_XTERM_GCONF_SCROLLBACK, sb, NULL);

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
terminal_settings_new (GtkWindow *parent)
{
    GtkWidget *dialog;
 
    dialog = g_object_new (TERMINAL_TYPE_SETTINGS, NULL);

    gtk_window_set_title(GTK_WINDOW(dialog), _("weba_ti_settings_title"));

#if 0
    if (parent) {
	  g_debug ("%s - Got parent", __FUNCTION__);
      gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	}
#endif

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

    g_debug (__FUNCTION__);

    /* Create dialog */
    GtkWidget *dialog = hildon_font_selection_dialog_new (NULL, NULL);
    if (dialog == NULL) {
      g_debug ("Couldn't create Dialog");
      return;
    }

    fontdesc = pango_font_description_from_string (
        gtk_font_button_get_font_name(GTK_FONT_BUTTON (settings->font_button)));
    if (fontdesc == NULL) {
      g_debug ("Couldn't create font description");
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
//  (void)data;

  update_shortcut_keys(data);
}
