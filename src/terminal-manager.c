#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)

#include "terminal-manager.h"
#include "terminal-window.h"

enum signals {
  S_NEW_WINDOW = 0,
  S_WINDOW_CLOSED,
  S_LAST_WINDOW,
  S_COUNT
};

static guint sigs[S_COUNT];

static void terminal_manager_window_destroy (TerminalWindow *window,
    					     TerminalManager *manager);
static void terminal_manager_window_new_window (TerminalWindow *window,
						const gchar *command,
    					        TerminalManager *manager);
static void terminal_manager_last_window_closed (TerminalManager *manager);
static void terminal_manager_global_state_changed (TerminalWindow *window,
						   TerminalManager *manager);

/* Helper */
static void _state_change_helper (gpointer window, gpointer data);

G_DEFINE_TYPE (TerminalManager, terminal_manager, HILDON_TYPE_PROGRAM);

TerminalManager *terminal_manager_get_instance (void)
{
  static TerminalManager *manager = NULL;

  if (!manager) {
    manager = g_object_new(TERMINAL_TYPE_MANAGER, NULL);
  }
  
  return manager;
}

static void terminal_manager_class_init (TerminalManagerClass *klass)
{
  klass->last_window_closed = terminal_manager_last_window_closed;

  sigs[S_NEW_WINDOW] = g_signal_new("new_window",
      				    TERMINAL_TYPE_MANAGER,
				    G_SIGNAL_RUN_LAST,
				    G_STRUCT_OFFSET(TerminalManagerClass,
						    new_window),
				    NULL,
				    NULL,
				    g_cclosure_marshal_VOID__OBJECT,
				    G_TYPE_NONE,
				    1, G_TYPE_OBJECT);
  sigs[S_WINDOW_CLOSED] = g_signal_new("window_closed",
      				       TERMINAL_TYPE_MANAGER,
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET(TerminalManagerClass,
						       window_closed),
				       NULL,
				       NULL,
				       g_cclosure_marshal_VOID__POINTER,
				       G_TYPE_NONE,
				       1, G_TYPE_POINTER);
  sigs[S_LAST_WINDOW] = g_signal_new("last_window_closed",
      				     TERMINAL_TYPE_MANAGER,
				     G_SIGNAL_RUN_LAST,
				     G_STRUCT_OFFSET(TerminalManagerClass,
						     last_window_closed),
				     NULL,
				     NULL,
				     g_cclosure_marshal_VOID__VOID,
				     G_TYPE_NONE,
				     0);
}

static void terminal_manager_init (TerminalManager *manager)
{
  manager->windows = NULL;
}

gboolean terminal_manager_new_window (TerminalManager *manager,
				  const gchar *command,
				  GError **error)
{
  TerminalWindow *window = TERMINAL_WINDOW(terminal_window_new());

  g_signal_connect(window,
		   "destroy",
		   G_CALLBACK(terminal_manager_window_destroy),
		   manager);
  g_signal_connect(window,
		   "new_window",
		   G_CALLBACK(terminal_manager_window_new_window),
		   manager);
  g_signal_connect (window, 
                    "global_state_changed", 
                    G_CALLBACK (terminal_manager_global_state_changed), 
                    manager);

  manager->windows = g_slist_append(manager->windows, window);
  g_object_set_data(G_OBJECT(window), "osso", g_object_get_data(G_OBJECT(manager), "osso"));

  hildon_program_add_window(HILDON_PROGRAM(manager), HILDON_WINDOW(window));

  if (!terminal_window_launch(window, command, error)) {
    manager->windows = g_slist_remove(manager->windows, window);
    gtk_widget_destroy(GTK_WIDGET(window));
    return FALSE;
  }

  terminal_window_set_state (window);

  return TRUE;
}

static void terminal_manager_window_destroy (TerminalWindow *window,
    					     TerminalManager *manager)
{
  manager->windows = g_slist_remove(manager->windows, window);

  g_signal_emit(manager, sigs[S_WINDOW_CLOSED], 0, window);

  if (!manager->windows) {
	  g_signal_emit(manager, sigs[S_LAST_WINDOW], 0);
  }

  /* HildonWindow will call hildon_program_remove_window on destroy. */
}

static void terminal_manager_last_window_closed (TerminalManager *manager)
{
  gtk_main_quit ();
}

static void terminal_manager_window_new_window (TerminalWindow *window,
						const gchar *command,
    					        TerminalManager *manager)
{
  GError *error = NULL;
  if (!terminal_manager_new_window(manager, command, &error)) {
    hildon_banner_show_information(GTK_WIDGET(window),
				   _("Unable to launch terminal: %s\n"),
				   error->message);
    g_error_free(error);
  }
}

static void terminal_manager_global_state_changed (TerminalWindow *window,
						   TerminalManager *manager)
{
  g_slist_foreach (manager->windows, (GFunc)_state_change_helper, NULL);
}

/* Helpers */
static void
_state_change_helper (gpointer window, gpointer data)
{
  terminal_window_set_state (window);
}

