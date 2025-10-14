/*
 * This file is a part of hildon-extras
 *
 * Copyright (C) 2010, 2025 Thomas Perl <m@thp.io>
 * Copyright (C) 2005, 2008 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version. or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * SECTION:he-simple_color-dialog
 * @short_description: An simple_color dialog for Hildon-based applications
 *
 * #HeSimpleColorDialog works as a nice default simple_color dialog for Maemo apps
 */

#define __USE_GNU       /* needed for locale */

#include <locale.h>

#include <string.h>
#include <stdlib.h>

#include <libintl.h>
#include <langinfo.h>

#include <hildon/hildon.h>

#include <gdk/gdk.h>

#include "he-simple-color-dialog.h"

#define PALETTE_SIZE 21
#define PALETTE_CUSTOM_COLOR (PALETTE_SIZE-1)

#define ICON_SIZE 50

static const gchar* DEFAULT_PALETTE[PALETTE_SIZE] = {
    "#fcaf3e", "#f57900", "#ce5c00", /* orange */
    "#8ae234", "#73d215", "#4e9a06", /* green */
    "#729fcf", "#3465a4", "#204a87", /* blue */
    "#ad7fa8", "#75507b", "#5c3566", /* purple */
    "#ef2929", "#cc0000", "#a40000", /* red */
    "#888a85", "#555753", "#2e3436", /* grey */
    "#ffffff", "#000000", "#ffffff", /* white, black, custom */
};

#define HE_SIMPLE_COLOR_DIALOG_GET_PRIVATE(obj)                           \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HE_TYPE_SIMPLE_COLOR_DIALOG, HeSimpleColorDialogPrivate))

#define _(String) dgettext("hildon-libs", String)

struct _HeSimpleColorDialogPrivate
{
    /* Widgets */
    GtkWidget* table_layout;
    GtkWidget* buttons[PALETTE_SIZE];

    /* Data */
    GdkColor palette[PALETTE_SIZE];
    guint color_index;
};

static GObject * he_simple_color_dialog_constructor (GType                  type,
                                                   guint                  n_construct_properties,
                                                   GObjectConstructParam *construct_properties);
static void he_simple_color_dialog_finalize (GObject * object);

void he_simple_color_dialog_response (GtkDialog *dialog, gint response_id, HeSimpleColorDialog *fd);
static void he_simple_color_dialog_class_init (HeSimpleColorDialogClass * class);
static void he_simple_color_dialog_init (HeSimpleColorDialog * fd);
static void he_simple_color_dialog_show (GtkWidget *widget);
static void he_simple_color_dialog_destroy (GtkObject *object);

static gpointer                                 parent_class = NULL;

GType G_GNUC_CONST
he_simple_color_dialog_get_type            (void)
{
    static GType dialog_type = 0;
	dialog_type = g_type_from_name ("HeSimpleColorDialog");

    if (!dialog_type) {
        static const GTypeInfo dialog_info =
        {
            sizeof (HeSimpleColorDialogClass),
            NULL,
            NULL,
            (GClassInitFunc) he_simple_color_dialog_class_init,
            NULL,
            NULL,
            sizeof (HeSimpleColorDialog),
            0,
            (GInstanceInitFunc) he_simple_color_dialog_init,
            NULL
        };

        dialog_type = g_type_register_static (GTK_TYPE_DIALOG, 
                "HeSimpleColorDialog", &dialog_info, 0);
    }

    return dialog_type;
}

static void
he_simple_color_dialog_class_init (HeSimpleColorDialogClass * class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  gobject_class = (GObjectClass *) class;
  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = g_type_class_peek_parent (class);

  g_type_class_add_private (object_class, sizeof (HeSimpleColorDialogPrivate));
}

static void
he_simple_color_dialog_show (GtkWidget *widget)
{
    /*HeSimpleColorDialogPrivate *priv = HE_SIMPLE_COLOR_DIALOG_GET_PRIVATE (widget);*/
    GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
he_simple_color_dialog_destroy (GtkObject *object)
{
    /*HeSimpleColorDialogPrivate *priv = HE_SIMPLE_COLOR_DIALOG_GET_PRIVATE (object);*/
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
_he_simple_color_dialog_toggled(GtkToggleButton* toggle_button, gpointer user_data)
{
    HeSimpleColorDialog* scd = HE_SIMPLE_COLOR_DIALOG(user_data);
    g_return_if_fail (HE_IS_SIMPLE_COLOR_DIALOG(scd));

    guint button_index = (guint)g_object_get_data(G_OBJECT(toggle_button), "color-index");

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button))) {
        if (scd->priv->color_index != button_index) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scd->priv->buttons[scd->priv->color_index]), FALSE);
            scd->priv->color_index = button_index;
        }
    }
}

static GtkWidget*
_he_simple_color_dialog_create_color_widget(GdkColor* color, int depth)
{
    GdkPixmap* pixmap = gdk_pixmap_new(NULL, ICON_SIZE, ICON_SIZE, depth);

    cairo_t* cr = gdk_cairo_create(pixmap);
    gdk_cairo_set_source_color(cr, color);

    cairo_rectangle(cr, 0, 0, ICON_SIZE, ICON_SIZE);
    cairo_fill(cr);
    cairo_destroy(cr);

    GtkWidget* result = gtk_image_new_from_pixmap(pixmap, NULL);
    gdk_pixmap_unref(pixmap);

    return result;
}

static int
_he_simple_color_dialog_get_screen_depth(void)
{
    GdkScreen *screen = gdk_screen_get_default();
    GdkVisual *visual = gdk_screen_get_system_visual(screen);
    return visual->depth;
}

static void
he_simple_color_dialog_init (HeSimpleColorDialog *ad)
{
    ad->priv = HE_SIMPLE_COLOR_DIALOG_GET_PRIVATE (ad);
    guint i;

    ad->priv->color_index = 0;

    /* Parse default colors */
    for (i=0; i<PALETTE_SIZE; i++) {
        if (!gdk_color_parse(DEFAULT_PALETTE[i], &(ad->priv->palette[i]))) {
            g_warning("Cannot parse color from default palette: %s (falling back to black)", DEFAULT_PALETTE[i]);
            ad->priv->palette[i].red = 0;
            ad->priv->palette[i].green = 0;
            ad->priv->palette[i].blue = 0;
        }
    }

    gtk_window_set_title(GTK_WINDOW(ad), _("Select a color"));

    /* Create table and insert color chooser buttons */
    GtkTable* t = GTK_TABLE(gtk_table_new(PALETTE_SIZE/3, 3, FALSE));

    int depth = _he_simple_color_dialog_get_screen_depth();
    for (i=0; i<PALETTE_SIZE; i++) {
        ad->priv->buttons[i] = hildon_gtk_toggle_button_new(HILDON_SIZE_THUMB_HEIGHT);
        gtk_widget_set_size_request(GTK_WIDGET(ad->priv->buttons[i]), 80, 80);
        gtk_container_add(GTK_CONTAINER(ad->priv->buttons[i]), _he_simple_color_dialog_create_color_widget(&(ad->priv->palette[i]), depth));
        g_object_set_data(G_OBJECT(ad->priv->buttons[i]), "color-index", (gpointer)i);
        g_signal_connect(G_OBJECT(ad->priv->buttons[i]), "toggled", G_CALLBACK(_he_simple_color_dialog_toggled), ad);
        /* Set color image on button */
        gtk_table_attach(GTK_TABLE(t), ad->priv->buttons[i],
                i/3, i/3+1, /* xattach */
                i%3, i%3+1, /* yattach */
                GTK_EXPAND | GTK_FILL, /* xoptions */
                GTK_EXPAND | GTK_FILL, /* yoptions */
                0, /* xpadding */
                0 /* ypadding */);
    }

    ad->priv->table_layout = GTK_WIDGET(t);
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(ad));
    gtk_container_add(GTK_CONTAINER(content_area), GTK_WIDGET(t));

    gtk_dialog_add_button(GTK_DIALOG(ad), _("Select"), GTK_RESPONSE_OK);

    /* By default, toggle the first button */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ad->priv->buttons[0]), TRUE);

    gtk_widget_show_all(GTK_WIDGET(ad));

    /* Hide the "custom color" button */
    gtk_widget_hide(GTK_WIDGET(ad->priv->buttons[PALETTE_CUSTOM_COLOR]));

    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(ad), GTK_NO_WINDOW);
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(ad), FALSE);
}

static void
he_simple_color_dialog_finalize (GObject * object)
{
  /*HeSimpleColorDialogPrivate *priv = HE_SIMPLE_COLOR_DIALOG_GET_PRIVATE (object);*/

  if (G_OBJECT_CLASS (parent_class)->finalize)
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* ------------------------------ PRIVATE METHODS ---------------------------- */

/* ------------------------------ PUBLIC METHODS ---------------------------- */

GtkWidget*
he_simple_color_dialog_new()
{
    return g_object_new(HE_TYPE_SIMPLE_COLOR_DIALOG, NULL);
}


/**
 * Sets the currently-selected color of this color dialog.
 * The color is copied to an internal structure, so the
 * caller can free the GdkColor passed to this object after
 * the function call.
 **/
void
he_simple_color_dialog_set_color(HeSimpleColorDialog* scd, GdkColor* color)
{
    g_return_if_fail (HE_IS_SIMPLE_COLOR_DIALOG(scd));
    guint i;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scd->priv->buttons[scd->priv->color_index]), FALSE);

    /* Try to find the color in the existing palette */
    for (i=0; i<PALETTE_SIZE; i++) {
        if (gdk_color_equal(color, &(scd->priv->palette[i]))) {
            scd->priv->color_index = i;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scd->priv->buttons[scd->priv->color_index]), TRUE);
            return;
        }
    }

    /* Replace the custom color field with the new color */
    scd->priv->color_index = PALETTE_CUSTOM_COLOR;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scd->priv->buttons[scd->priv->color_index]), TRUE);
    scd->priv->palette[scd->priv->color_index] = *color;

    /* Update button image */
    GtkWidget* old_child = gtk_bin_get_child(GTK_BIN(scd->priv->buttons[scd->priv->color_index]));
    gtk_widget_destroy(old_child);
    gtk_container_add(GTK_CONTAINER(scd->priv->buttons[scd->priv->color_index]), _he_simple_color_dialog_create_color_widget(color, _he_simple_color_dialog_get_screen_depth()));

    /* Show the custom button now that we have a color set */
    gtk_widget_show_all(GTK_WIDGET(scd->priv->buttons[scd->priv->color_index]));
}

/**
 * Returns the currently-selected color. The caller has to
 * free the returned value using gdk_color_free.
 **/
GdkColor*
he_simple_color_dialog_get_color(HeSimpleColorDialog* scd)
{
    g_assert(HE_IS_SIMPLE_COLOR_DIALOG(scd));
    return gdk_color_copy(&(scd->priv->palette[scd->priv->color_index]));
}

GtkWidget *
he_simple_color_dialog_helper_create_color_widget(GdkColor *color)
{
    return _he_simple_color_dialog_create_color_widget(color,
            _he_simple_color_dialog_get_screen_depth());
}


