#ifndef __TERMINAL_ENCODING_H__
#define __TERMINAL_ENCODING_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "terminal-widget.h"

typedef struct _TerminalEncoding TerminalEncoding;

struct _TerminalEncoding
{
  gchar *name;
  const gchar *encoding;
};

GSList *terminal_encoding_get_list (void);
void terminal_encoding_free_list (GSList *list);
gchar *terminal_encoding_dialog (TerminalWidget *terminal, GtkWindow *parent, 
				 const gchar *defenc);

gchar *terminal_encoding_get_name_to_encoding (const gchar *encoding);
#endif
