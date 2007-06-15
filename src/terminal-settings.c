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

#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)

#include "terminal-settings.h"


static void terminal_settings_class_init    (TerminalSettingsClass *klass);
static void terminal_settings_init          (TerminalSettings      *header);
static void terminal_settings_finalize      (GObject               *object);


struct _TerminalSettings
{
  GtkDialog    __parent__;

  GtkWidget   *table;
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
    settings->table = gtk_table_new(2, 2, FALSE);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(settings)->vbox), settings->table);

    gtk_dialog_add_buttons(GTK_DIALOG(settings),
                           GTK_STOCK_OK, GTK_RESPONSE_OK,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           NULL);

    g_signal_connect_swapped(GTK_WIDGET(settings),
                             "response",
                             G_CALLBACK(gtk_widget_destroy),
                             settings);

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
