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

#include <gconf/gconf-client.h>
#include <libintl.h>
#include <locale.h>
#include <stdlib.h>
#include <hildon-widgets/hildon-color-button.h>
#define _(String) gettext(String)

#include "terminal-gconf.h"
#include "terminal-settings.h"


static void terminal_settings_class_init    (TerminalSettingsClass *klass);
static void terminal_settings_init          (TerminalSettings      *header);
static void terminal_settings_finalize      (GObject               *object);


struct _TerminalSettings
{
  GtkDialog    __parent__;

  GtkWidget   *font_button;
  GtkWidget   *fg_button;
  GtkWidget   *bg_button;
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
//    settings->table = gtk_table_new(2, 2, FALSE);

 //   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), settings->table);
    GConfClient *gc = gconf_client_get_default();
    GdkColor fg;
    GdkColor bg;

    gchar *color_name;
    gchar *font_name = gconf_client_get_string(gc, OSSO_XTERM_GCONF_FONT_NAME, NULL);
    gchar *font = NULL;

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
	    gdk_color_parse(color_name, &fg);
	    g_free(color_name);
    }

    color_name = gconf_client_get_string(gc, OSSO_XTERM_GCONF_BG_COLOR, NULL);
    if (!color_name) {
	    gdk_color_parse(OSSO_XTERM_DEFAULT_BG_COLOR, &bg);
    } else {
	    gdk_color_parse(color_name, &bg);
	    g_free(color_name);
    }

    g_object_unref(gc);

    font = g_strdup_printf("%s %d", font_name, font_size);
    g_free(font_name);
    font_name = NULL;

    settings->font_button = gtk_font_button_new_with_font(font);
    g_free(font);
    font = NULL;

    settings->fg_button = hildon_color_button_new_with_color(&fg);
    settings->bg_button = hildon_color_button_new_with_color(&bg);

    gtk_container_add(GTK_CONTAINER(GTK_CONTAINER(GTK_DIALOG(settings)->vbox)), settings->font_button);
    gtk_container_add(GTK_CONTAINER(GTK_CONTAINER(GTK_DIALOG(settings)->vbox)), settings->fg_button);
    gtk_container_add(GTK_CONTAINER(GTK_CONTAINER(GTK_DIALOG(settings)->vbox)), settings->bg_button);

    gtk_dialog_add_buttons(GTK_DIALOG(settings),
                           "Ok", GTK_RESPONSE_OK,
                           "Cancel", GTK_RESPONSE_CANCEL,
                           NULL);

    gtk_widget_show_all(GTK_WIDGET(settings));
}


static void
terminal_settings_finalize (GObject *object)
{
    parent_class->finalize (object);
}


gboolean
terminal_settings_store (TerminalSettings *settings)
{
    GConfClient *gc = gconf_client_get_default();
    const gchar *font = gtk_font_button_get_font_name(GTK_FONT_BUTTON(settings->font_button));
    const gchar *sep = g_utf8_strrchr(font, -1, ' ');
    gchar *color_name;
    GdkColor *color;

    if (!sep) return FALSE;

    gchar *font_name = g_strndup(font, (sep - font));

    gconf_client_set_string(gc, OSSO_XTERM_GCONF_FONT_NAME, font_name, NULL);
    gconf_client_set_int(gc, OSSO_XTERM_GCONF_FONT_BASE_SIZE, atoi(sep + 1), NULL);

    g_free(font_name);

    color = hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->fg_button));
    color_name = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
    gconf_client_set_string(gc, OSSO_XTERM_GCONF_FONT_COLOR, color_name, NULL);
    g_free(color_name);

    color = hildon_color_button_get_color(HILDON_COLOR_BUTTON(settings->bg_button));
    color_name = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
    gconf_client_set_string(gc, OSSO_XTERM_GCONF_BG_COLOR, color_name, NULL);
    g_free(color_name);

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

    gtk_window_set_title(GTK_WINDOW(dialog), _("X Terminal Settings"));

    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

    return dialog;
}
