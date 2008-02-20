#include <glib.h>
#include <glib-object.h>
#include <hildon/hildon-program.h>
#include "terminal-window.h"

#ifndef TERMINAL_MANAGER_H
#define TERMINAL_MANAGER_H

G_BEGIN_DECLS;

#define TERMINAL_TYPE_MANAGER            (terminal_manager_get_type ())
#define TERMINAL_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_MANAGER, TerminalManager))
#define TERMINAL_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_MANAGER, TerminalManagerClass))
#define TERMINAL_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_MANAGER))
#define TERMINAL_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_MANAGER))
#define TERMINAL_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_MANAGER, TerminalManagerClass))

typedef struct _TerminalManagerClass TerminalManagerClass;
typedef struct _TerminalManager      TerminalManager;

struct _TerminalManagerClass
{
  HildonProgramClass __parent__;

  void (*new_window)(TerminalManager *manager, TerminalWindow *window);
  void (*window_closed)(TerminalManager *manager, TerminalWindow *window);
  void (*last_window_closed)(TerminalManager *manager);
};

struct _TerminalManager
{
  HildonProgram __parent__;

  GSList *windows;
};

GType            terminal_manager_get_type (void) G_GNUC_CONST;
TerminalManager *terminal_manager_get_instance (void);

gboolean         terminal_manager_new_window (TerminalManager *manager,
					      const gchar *command,
					      GError **error);

G_END_DECLS;

#endif /* TERMINAL_MANAGER_H */
