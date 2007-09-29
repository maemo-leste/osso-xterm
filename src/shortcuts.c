#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#define GETTEXT_PACKAGE "osso-browser-ui"
#include <glib/gi18n-lib.h>

#include <gconf/gconf-client.h>

#if HILDON == 0
#include <hildon-widgets/hildon-caption.h>
#elif HILDON == 1
#include <hildon/hildon-caption.h>
#endif
#include <libintl.h>
#include "terminal-gconf.h"
#include "shortcuts.h"

#include <assert.h>

//#define _(x) gettext(x)

enum Responses {
	GRAPH_RESPONSE_NEW = 1,
	GRAPH_RESPONSE_EDIT,
	GRAPH_RESPONSE_DELETE,
	GRAPH_RESPONSE_DONE,
};

typedef struct {
	GConfClient *gcc;
	GtkWidget *view_up;
	GtkWidget *view_down;
	GtkDialog *keys_dialog;
	GtkWidget *keys_list;
} GraphApplet;

static void ui_create_main_dialog(GraphApplet *applet, gpointer window);
static void keys_dialog_response(GtkDialog *dialog, gint response,
	       	GraphApplet *applet);
static gboolean key_dialog_run(GtkWindow *parent, gchar **title, gchar **key);
static void update_cmds(GtkTreeModel *model, GConfClient *gcc);
static void selection_changed(GtkTreeSelection *sel, GraphApplet *applet);
static void move_up(GtkButton *button, GraphApplet *applet);
static void move_down(GtkButton *button, GraphApplet *applet);

void update_shortcut_keys(gpointer window)
{
	GraphApplet applet;

/*	(void) bindtextdomain(PACKAGE, LOCALEDIR); */

	/* Create the main dialog and refresh the ui */

	ui_create_main_dialog(&applet, window);

	gtk_dialog_run(GTK_DIALOG(applet.keys_dialog));
	update_cmds(gtk_tree_view_get_model(GTK_TREE_VIEW(applet.keys_list)),
			applet.gcc);

	/* Make un-initializations */
	gtk_widget_destroy(GTK_WIDGET(applet.keys_dialog));
}

static void ui_create_main_dialog(GraphApplet *applet, gpointer window)
{
	GSList *l_cmds;
	GSList *l_cmd_nms;
	GSList *iter, *iter_n;

	GtkListStore *model;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;
	GtkWidget *view_hbox;
	GtkWidget *view_vbox;

	applet->gcc = gconf_client_get_default();

	l_cmds = gconf_client_get_list(applet->gcc,
			OSSO_XTERM_GCONF_KEYS,
			GCONF_VALUE_STRING,
			NULL);

	l_cmd_nms = gconf_client_get_list(applet->gcc,
			OSSO_XTERM_GCONF_KEY_LABELS,
			GCONF_VALUE_STRING,
			NULL);

	applet->keys_dialog = GTK_DIALOG(
			gtk_dialog_new_with_buttons(_("weba_fi_plugin_details_shortcut"),
			GTK_WINDOW(window),
			GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
			_("webb_me_new"), GRAPH_RESPONSE_NEW,
			_("webb_me_edit"), GRAPH_RESPONSE_EDIT,
			_("webb_me_delete"), GRAPH_RESPONSE_DELETE,
			_("webb_me_close"), GRAPH_RESPONSE_DONE,
			NULL));

//    gtk_window_set_transient_for(GTK_WINDOW(applet->keys_dialog), GTK_WINDOW (window));
    gtk_window_set_modal(GTK_WINDOW (applet->keys_dialog), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(applet->keys_dialog), TRUE);

	model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

	for (iter = l_cmds, iter_n = l_cmd_nms; iter && iter_n; 
			iter = iter->next, iter_n = iter_n->next) {
		GtkTreeIter titer;
		gtk_list_store_append(model, &titer);
		gtk_list_store_set(model, &titer, 0, iter_n->data,
				1, iter->data, -1);
	}

	applet->keys_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(applet->keys_list), FALSE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->keys_list)), "changed", G_CALLBACK(selection_changed), applet);
	g_object_unref(model);

	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("weba_fi_page_details_page_title"), rend,
			"text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(applet->keys_list), col);

	view_hbox = gtk_hbox_new(FALSE, 0);
	view_vbox = gtk_vbox_new(FALSE, 0);

//	applet->view_up = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
//	applet->view_down = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);

	GtkWidget *arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_EXPAND);
    gtk_widget_show (arrow);
	applet->view_up = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (applet->view_up), arrow);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_EXPAND);
    gtk_widget_show (arrow);
	applet->view_down = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (applet->view_down), arrow);

	g_signal_connect(applet->view_up, "clicked", G_CALLBACK(move_up), applet);
	g_signal_connect(applet->view_down, "clicked", G_CALLBACK(move_down), applet);

	g_slist_foreach(l_cmds, (GFunc)g_free, NULL);
	g_slist_foreach(l_cmd_nms, (GFunc)g_free, NULL);

	g_slist_free(l_cmds);
	g_slist_free(l_cmd_nms);

	gtk_box_pack_start(GTK_BOX(view_hbox), applet->keys_list,
			TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(view_hbox), view_vbox,
			FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(view_vbox), applet->view_up);
	gtk_container_add(GTK_CONTAINER(view_vbox), applet->view_down);

	gtk_container_add(GTK_CONTAINER(applet->keys_dialog->vbox),
		       	view_hbox);

	g_signal_connect(applet->keys_dialog, "response",
			G_CALLBACK(keys_dialog_response), applet);

	gtk_widget_show_all(GTK_WIDGET(applet->keys_dialog));
	gtk_widget_set_size_request (GTK_WIDGET (applet->keys_dialog), 400, 250);
}

static void keys_dialog_response(GtkDialog *dialog, gint response,
		GraphApplet *applet)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	gchar *title = NULL;
	gchar *key = NULL;
	switch (response) {
		case GRAPH_RESPONSE_DONE:
			return;
			break;
		case GRAPH_RESPONSE_NEW:
			if (key_dialog_run(GTK_WINDOW(applet->keys_dialog), &title, &key)) {
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(applet->keys_list));
				gtk_list_store_append(GTK_LIST_STORE(model), &iter);
				gtk_list_store_set(GTK_LIST_STORE(model),
						&iter,
						0, title,
						1, key,
						-1);
			}
			break;
		case GRAPH_RESPONSE_EDIT:
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->keys_list));
			if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
				gtk_tree_model_get(model, &iter, 0, &title,
						1, &key, -1);
				if (key_dialog_run(GTK_WINDOW(applet->keys_dialog), &title, &key)) {
					gtk_list_store_set(GTK_LIST_STORE(model),
							&iter,
							0, title,
							1, key,
							-1);
				}
			}
			break;
		case GRAPH_RESPONSE_DELETE:
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->keys_list));
			if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
				gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			}
			break;
	}
	if (title) g_free(title);
	if (key) g_free(key);
	g_signal_stop_emission_by_name(dialog, "response");
}

static gboolean key_dialog_run(GtkWindow *parent, gchar **title, gchar **key)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit key"),
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
			_("Done"), GTK_RESPONSE_ACCEPT,
			_("Cancel"), GTK_RESPONSE_REJECT,
			NULL);
	GtkWidget *title_entry = gtk_entry_new();
	GtkWidget *key_entry = gtk_entry_new();
	GtkSizeGroup *group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	GtkWidget *title_caption = hildon_caption_new(group,
			_("weba_fi_page_details_page_title"),
			title_entry,
			NULL,
			HILDON_CAPTION_OPTIONAL);
	GtkWidget *key_caption = hildon_caption_new(group,
			_("weba_fi_cookie_details_value"),
			key_entry,
			NULL,
			HILDON_CAPTION_OPTIONAL);

	if (*title) {
		gtk_entry_set_text(GTK_ENTRY(title_entry), *title);
	}
	if (*key) {
		gtk_entry_set_text(GTK_ENTRY(key_entry), *key);
	}

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
			title_caption);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
			key_caption);

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free(*title);
		*title = g_strdup(gtk_entry_get_text(GTK_ENTRY(title_entry)));
		*key = g_strdup(gtk_entry_get_text(GTK_ENTRY(key_entry)));
		gtk_widget_destroy(dialog);
		return TRUE;
	}
	
	gtk_widget_destroy(dialog);
	return FALSE;
}

static void update_cmds(GtkTreeModel *model, GConfClient *gcc)
{
	GSList *l_cmd = NULL;
	GSList *l_cmd_nm = NULL;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		gchar *cmd, *name;

		do {
			gtk_tree_model_get(model, &iter, 0, &name, 1, &cmd, -1);
			l_cmd = g_slist_append(l_cmd, cmd);
			l_cmd_nm = g_slist_append(l_cmd_nm, name);
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	gconf_client_set_list(gcc, OSSO_XTERM_GCONF_KEY_LABELS,
			GCONF_VALUE_STRING, l_cmd_nm, NULL);
	gconf_client_set_list(gcc, OSSO_XTERM_GCONF_KEYS,
			GCONF_VALUE_STRING, l_cmd, NULL);

	g_slist_foreach(l_cmd, (GFunc)g_free, NULL);
	g_slist_foreach(l_cmd_nm, (GFunc)g_free, NULL);
	g_slist_free(l_cmd);
	g_slist_free(l_cmd_nm);
}

static void selection_changed(GtkTreeSelection *sel, GraphApplet *applet)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			gtk_widget_set_sensitive(applet->view_up, TRUE);
		} else {
			gtk_widget_set_sensitive(applet->view_up, FALSE);
		}
		gtk_tree_path_free(path);
		if (gtk_tree_model_iter_next(model, &iter)) {
			gtk_widget_set_sensitive(applet->view_down, TRUE);
		} else {
			gtk_widget_set_sensitive(applet->view_down, FALSE);
		}
	}
}

static void move_up(GtkButton *button, GraphApplet *applet)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->keys_list));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			GtkTreeIter iter2;
			gtk_tree_model_get_iter(model, &iter2, path);
			gtk_list_store_move_before(GTK_LIST_STORE(model),
					&iter,
					&iter2);
		}
		gtk_tree_path_free(path);
		g_signal_emit_by_name(selection, "changed");
	}
}

static void move_down(GtkButton *button, GraphApplet *applet)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->keys_list));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreeIter iter2 = iter;
		if (gtk_tree_model_iter_next(model, &iter2)) {
			gtk_list_store_move_after(GTK_LIST_STORE(model),
					&iter,
					&iter2);
		}
		g_signal_emit_by_name(selection, "changed");
	}
}
