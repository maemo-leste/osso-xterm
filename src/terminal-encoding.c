#include <glib.h>
#include <vte/vte.h>

#define GETTEXT_PACKAGE "osso-browser-ui"
#include <glib/gi18n-lib.h>
#include <libintl.h>

#include "terminal-encoding.h"

#define TEST_STRING "assdfghhjy"
#define TEST_STRING_LEN 10

static GtkWidget *create_tree_from_list (GSList *list, const gchar *curenc);
static void selection_changed(GtkTreeSelection *sel, GtkListStore *model2);
static void add_encoding_if_suitable (TerminalEncoding *encoding);

static TerminalEncoding selected_encoding;
static GSList *enc = NULL;

/* Array of tested encodings */
static TerminalEncoding encodings[] = {
  /* name , encoding */
  { NULL, "UTF-8"},

  { NULL, "ISO-8859-1"},
  { NULL, "ISO-8859-15"},
  { NULL, "ISO-8859-2"},
  { NULL, "ISO-8859-7"},
  { NULL, "ISO-8859-9"},

  { NULL, "BIG5"},

  { NULL, "CP1250"},
  { NULL, "CP1251"},
  { NULL, "CP1253"},
  { NULL, "CP1254"},

  { NULL, "EUC-TW"},

  { NULL, "KOI8-R"},
};

/**
 * name_to_encode_index
 *
 * @index : index of encoding
 *
 * Return value : localized name of encoding
 */
static gchar *
name_to_encode_index (guint index)
{
  gchar *retval = NULL;
  switch (index) {
  case 0:
    retval = _("weba_va_encoding_utf_8");
    break;
  case 1:
    retval = _("weba_va_encoding_iso_8859_1");
    break;
  case 2:
    retval = _("weba_va_encoding_iso_8859_15");
    break;
  case 3:
    retval = _("weba_va_encoding_iso_8859_2");
    break;
  case 4:
    retval = _("weba_va_encoding_iso_8859_7");
    break;
  case 5:
    retval = _("weba_va_encoding_iso_8859_9");
    break;
  case 6:
    retval = _("weba_va_encoding_big5");
    break;
  case 7:
    retval = _("weba_va_encoding_win_1250");
    break;
  case 8:
    retval = _("weba_va_encoding_win_1251");
    break;
  case 9:
    retval = _("weba_va_encoding_win_1253");
    break;
  case 10:
    retval = _("weba_va_encoding_win_1254");
    break;
  case 11:
    retval = _("weba_va_encoding_euc_tw");
    break;
  case 12:
    retval = _("weba_va_encoding_win_koi_8r");
    break;
  default:
    break;
  }

  return g_strdup (retval);
}

/**
 * terminal_encoding_get_list
 * 
 * Return value : GSList of encodings
 */
GSList *
terminal_encoding_get_list (void)
{
  guint i = 0;
  guint j = sizeof (encodings)/sizeof (TerminalEncoding);

  if (enc == NULL) {
    for (i = 0; i < j; i++ )  {
      encodings[i].name = name_to_encode_index (i);
      add_encoding_if_suitable (&encodings[i]);
    }
  }
  return enc;
}

/**
 * terminal_encoding_free_list
 * 
 * list : GSList of encodings
 */
void
terminal_encoding_free_list (GSList *list)
{
  guint i = 0;
  guint j = sizeof (encodings)/sizeof (TerminalEncoding);

  if (enc != NULL) {
    for (i = 0; i < j; i++ )  {
      g_free (encodings[i].name);
    }
  }
  g_slist_free (list);
  list = NULL;
  enc = NULL;
}

static void
add_encoding_if_suitable (TerminalEncoding *encoding)
{
  GError *error = NULL;
  gchar *converted = NULL;
  gsize bytes_read = 0;
  gsize bytes_written = 0;

  g_assert (encoding != NULL);

  /* Test available codesets. Fix if there is easier way to do this */
  converted = g_convert (TEST_STRING, TEST_STRING_LEN,
                         encoding->encoding,
                         "UTF-8",
                         &bytes_read,
                         &bytes_written,
                         &error);

  /* Only success/nonsuccess is intresting */
  if (error != NULL) {
    g_clear_error (&error);
    return;
  }
  g_free (converted);

  /* Add this encoding to list */
  enc = g_slist_append (enc, encoding);

}

static GtkWidget *
create_tree_from_list (GSList *list, const gchar *curenc)
{
  GtkListStore *model;
  GtkWidget *tree = NULL;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;
  GtkTreeRowReference *rowref = NULL;

  model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  for (list = g_slist_nth (list, 0); list != NULL; 
       list = g_slist_next (list)) {
    GtkTreeIter titer;
    TerminalEncoding *enc = (TerminalEncoding *)list->data;
    if (enc == NULL) {
      continue;
    }

    gtk_list_store_append (model, &titer);
    gtk_list_store_set (model, &titer, 0, enc->name, 1, enc->encoding, -1);
    if (!g_ascii_strcasecmp (enc->encoding, curenc)) {
      GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &titer);
      rowref = gtk_tree_row_reference_new (GTK_TREE_MODEL (model), path);
      gtk_tree_path_free (path);
    }
  }

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(tree), FALSE);

  g_signal_connect (gtk_tree_view_get_selection ( GTK_TREE_VIEW(tree)), 
		    "changed", G_CALLBACK (selection_changed), model);

  g_object_unref(model);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes("poks", rend, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes("poks", rend, "text", 1, NULL);
  g_object_set (col, "visible", FALSE, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);

  /* Select current charset */
  {
    GtkTreeIter siter;
    GtkTreePath *tpath;

    tpath = gtk_tree_row_reference_get_path (rowref);

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &siter, tpath)) {
      GtkTreeSelection *selection;
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
      gtk_tree_selection_select_iter (selection, &siter);

      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree), tpath, NULL, TRUE,
				    0.0, 0.0);
      gtk_tree_row_reference_free (rowref);
    }

  }

  return tree;
}

/**
 * terminal_encoding_get_name_to_encoding
 *
 * @encoding : predefined name of from TerminalEncoding
 *
 * Return value : localized name of encoding
 */
gchar *
terminal_encoding_get_name_to_encoding (const gchar *encoding)
{
  GSList *list = NULL;

  if (encoding == NULL) {
    return NULL;
  }

  list = terminal_encoding_get_list ();
  for (list = g_slist_nth (list, 0); list != NULL; 
       list = g_slist_next (list)) {
    TerminalEncoding *enc = (TerminalEncoding *)list->data;
    if (enc == NULL) {
      continue;
    }
    if (!g_ascii_strcasecmp (encoding, enc->encoding)) {
      return enc->name;
    }
  }  
  return NULL;
}

/**
 * terminal_encoding_dialog
 *
 * @terminal : Terminal widget
 * @parent : Dialogs parent-window
 * @defenc : default encoding
 *
 * Return value : encoding of current codeset
 */
gchar *
terminal_encoding_dialog (TerminalWidget *terminal, GtkWindow *parent, 
			  const gchar *defenc)
{
  GSList *list = NULL;
  GtkWidget *dialog = NULL;
  GtkWidget *tree = NULL;
  GtkWidget *scrolled = NULL;

  list = terminal_encoding_get_list ();

  dialog = gtk_dialog_new_with_buttons(_("weba_ti_encoding_title"),
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
			_("weba_bd_ok"), GTK_RESPONSE_ACCEPT,
			_("weba_bd_cancel"), GTK_RESPONSE_REJECT,
			NULL);

  tree = create_tree_from_list (list, defenc);
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolled), tree);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scrolled);

  gtk_widget_set_size_request (scrolled, 350, 150);

  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    if (terminal != NULL) {
      g_object_set (terminal, "encoding", selected_encoding.encoding, NULL);
    }
    gtk_widget_destroy (dialog);
    return (gchar *)selected_encoding.encoding;
  }
  
  gtk_widget_destroy (dialog);
  return (gchar *)defenc;
 
}

static void 
selection_changed(GtkTreeSelection *sel, GtkListStore *model2)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
    gtk_tree_model_get (model, &iter,
			0, &selected_encoding.name,
			1, &selected_encoding.encoding,
			-1);
  }
}
