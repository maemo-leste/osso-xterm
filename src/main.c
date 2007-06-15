/* $Id: main.c 4425 2005-09-16 08:09:49Z hedberg $ */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
/*#include <dbus/dbus-glib-lowlevel.h> */

#include <libosso.h>

#include "terminal-app.h"

int
main (int argc, char **argv)
{
  gpointer         app;
  GError          *error = NULL;
  osso_context_t  *osso_context;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

#ifdef DEBUG
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#endif

  g_set_application_name (_("X Terminal"));

  gtk_init (&argc, &argv);

  app = terminal_app_new();
  g_object_add_weak_pointer(G_OBJECT(app), &app);

  osso_context = osso_initialize("xterm", VERSION, FALSE, NULL);

  if (osso_context == NULL) {
      g_printerr("osso_initialize() failed!\n");
      exit(EXIT_FAILURE);
  }

  g_object_set_data(G_OBJECT(app), "osso", osso_context);
  if (!terminal_app_launch (TERMINAL_APP(app), &error))
    {
      g_printerr (_("Unable to launch terminal: %s\n"), error->message);
      g_error_free(error);
      exit(EXIT_FAILURE);
    }

  gtk_main ();

  if (app != NULL)
    {
      g_object_unref(G_OBJECT(app));
    }

  osso_deinitialize(osso_context);

  return EXIT_SUCCESS;
}
