#include <string.h>
#include <vte/vte.h>
#include <gconf/gconf-client.h>
#include <hildon/hildon.h>
#include "font-dialog.h"
#include "terminal-gconf.h"

#define PREVIEW_TEXT "drwxr-xr-x 2 user users\r\n-rwxr-xr-x 1 user users"

enum {
  FONT_NAME_STRING_COLUMN,
  FONT_NAME_FAMILY_COLUMN,
  FONT_NAME_FACE_COLUMN,
  FONT_NAME_PFD_COLUMN
};

enum {
  FONT_SIZE_STRING_COLUMN,
  FONT_SIZE_INT_COLUMN
};

typedef struct
{
  GtkDialog *dlg;
  VteTerminal *preview;

  GtkTreeView *tv_size;
  GtkTreeModel *tm_size;
  GtkTreeSelection *sel_size;

  GtkTreeView *tv_name;
  GtkTreeModel *tm_name;
  GtkTreeSelection *sel_name;

  GtkWidget *fg_clr;
  GtkWidget *bg_clr;
} FontDialog;

static FontDialog font_dialog = { NULL };

static const guint8 font_sizes[] = {6, 8, 10, 12, 16, 24, 32};

static void
tv_realize(GtkTreeView *tv, GtkWidget *dlg)
{
  GtkTreeIter itr;
  GtkTreeModel *tm;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);

  if (gtk_tree_selection_get_selected(sel, &tm, &itr)) {
    GtkTreePath *tp = gtk_tree_model_get_path(tm, &itr);
    GdkRectangle rc;
    gint y;

    gtk_tree_view_get_background_area(tv, tp, NULL, &rc);
    gtk_tree_view_convert_bin_window_to_tree_coords(tv, 0, rc.y, NULL, &y);
    hildon_pannable_area_scroll_to(HILDON_PANNABLE_AREA(gtk_widget_get_parent(GTK_WIDGET(tv))), -1, y + (rc.height >> 1));
  }

  g_signal_handlers_disconnect_by_func(G_OBJECT(tv), (GCallback)tv_realize, dlg);
  if (!GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tv), "have-show-handler"))) {
    g_signal_connect_swapped(G_OBJECT(dlg), "show", (GCallback)tv_realize, tv);
    g_object_set_data(G_OBJECT(tv), "have-show-handler", GINT_TO_POINTER(1));
  }
}

static void
select_iter(GtkTreeView *tv, GtkTreeModel *tm, GtkTreeIter *itr)
{
  GtkTreePath *tp = gtk_tree_model_get_path(tm, itr);

  gtk_tree_selection_select_iter(gtk_tree_view_get_selection(tv), itr);
  gtk_tree_path_free(tp);
}

static void
select_font(char *name, int size, FontDialog *fd)
{
  gboolean do_break = FALSE;
  GtkTreeIter itr;
  GtkTreeModel *tm;
  char *str = g_strdup_printf("%s %d", name, size);
  PangoFontDescription *pfd = pango_font_description_from_string(str),
                       *pfd_list = NULL; 
  PangoFontFamily *family = NULL;
  PangoFontFace *face = NULL;
  int list_size = 0;

  tm = gtk_tree_view_get_model(fd->tv_name);
  if (gtk_tree_model_get_iter_first(tm, &itr))
    do {
      gtk_tree_model_get(tm, &itr, FONT_NAME_FAMILY_COLUMN, &family, FONT_NAME_FACE_COLUMN, &face, -1);
      if (!strcmp(pango_font_family_get_name(family), pango_font_description_get_family(pfd))) {
        pfd_list = pango_font_face_describe(face);
        do_break = ((pango_font_description_get_weight(pfd_list)  == pango_font_description_get_weight(pfd)  &&
                     pango_font_description_get_style(pfd_list)   == pango_font_description_get_style(pfd)   &&
                     pango_font_description_get_stretch(pfd_list) == pango_font_description_get_stretch(pfd) &&
                     pango_font_description_get_variant(pfd_list) == pango_font_description_get_variant(pfd)));
        pango_font_description_free(pfd_list);
        if (do_break) break;
      }
    } while (gtk_tree_model_iter_next(tm, &itr));

  if (do_break)
    select_iter(fd->tv_name, tm, &itr);

  pango_font_description_free(pfd);
  g_free(str);

  do_break = FALSE;
  tm = gtk_tree_view_get_model(fd->tv_size);
  if (gtk_tree_model_get_iter_first(tm, &itr))
    do {
      gtk_tree_model_get(tm, &itr, FONT_SIZE_INT_COLUMN, &list_size, -1);
      do_break = (list_size == size);
      if (do_break) break;
    } while (gtk_tree_model_iter_next(tm, &itr));

  if (do_break)
    select_iter(fd->tv_size, tm, &itr);
}

static void
sel_changed(GtkTreeSelection *sel, FontDialog *fd)
{
  PangoFontDescription *pfd = NULL;
  GtkTreeIter itr;
  int size;

  if (gtk_tree_selection_get_selected(fd->sel_size, NULL, &itr)) {
    gtk_tree_model_get(fd->tm_size, &itr, FONT_SIZE_INT_COLUMN, &size, -1);

    if (gtk_tree_selection_get_selected(fd->sel_name, NULL, &itr))
      gtk_tree_model_get(fd->tm_name, &itr, FONT_NAME_PFD_COLUMN, &pfd, -1);
  }

  if (pfd != NULL) {
    if (size > 0) {
      pango_font_description_set_size(pfd, size * PANGO_SCALE);
      vte_terminal_reset(fd->preview, TRUE, TRUE);
      vte_terminal_feed(fd->preview, PREVIEW_TEXT, strlen(PREVIEW_TEXT));
      vte_terminal_set_font(fd->preview, pfd);
    }
    pango_font_description_free(pfd);
  }
}

static void
clr_changed(GObject *btn, GParamSpec *pspec, FontDialog *fd)
{
  GdkColor *clr_fg = NULL, *clr_bg = NULL;

  g_object_get(G_OBJECT(fd->fg_clr), "color", &clr_fg, NULL);
  g_object_get(G_OBJECT(fd->bg_clr), "color", &clr_bg, NULL);

  g_print("clr_changed: fg = (%04x, %04x, %04x), bg = (%04x, %04x, %04x)\n",
    clr_fg->red, clr_fg->green, clr_fg->blue,
    clr_bg->red, clr_bg->green, clr_bg->blue);

  vte_terminal_set_color_foreground(fd->preview, clr_fg);
  vte_terminal_set_color_background(fd->preview, clr_bg);
  gdk_color_free(clr_fg);
  gdk_color_free(clr_bg);
}

static void
preview_realize(GtkWidget *preview, FontDialog *fd)
{
  g_object_notify(G_OBJECT(fd->fg_clr), "color");
}

static void
create_font_dialog(FontDialog *fd)
{
  int Nix;
  char *str;
  GtkTreeIter itr_size;
  GtkWidget
    *hbox = g_object_new(GTK_TYPE_HBOX, "visible", TRUE, "spacing", 8, "border-width", 8, NULL),
    *pan;
  GtkListStore
    *ls_name = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_OBJECT, PANGO_TYPE_FONT_DESCRIPTION),
    *ls_size = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  GtkCellLayout *cl;
  GtkCellRenderer *cr;

  fd->dlg = GTK_DIALOG(gtk_dialog_new_with_buttons(g_dgettext("gtk20", "Pick a Font"), NULL, GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL));
  g_object_set(G_OBJECT(fd->dlg->vbox), "spacing", 0, NULL);

  fd->preview = g_object_new(VTE_TYPE_TERMINAL,  "visible", TRUE, "sensitive", FALSE, NULL);
  vte_terminal_set_scrollback_lines(fd->preview, 0);
  vte_terminal_feed(fd->preview, PREVIEW_TEXT, strlen(PREVIEW_TEXT));
  g_signal_connect(G_OBJECT(fd->preview), "realize", (GCallback)preview_realize, fd);

  fd->tm_name = GTK_TREE_MODEL(ls_name);
  fd->tv_name = g_object_new(GTK_TYPE_TREE_VIEW, "visible", TRUE, "hildon-ui-mode", HILDON_UI_MODE_EDIT, "model", ls_name, NULL);
  fd->sel_name = gtk_tree_view_get_selection(fd->tv_name);
  g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(fd->tv_name)), "changed", (GCallback)sel_changed, fd);
  cr = gtk_cell_renderer_text_new();
  cl = GTK_CELL_LAYOUT(gtk_tree_view_column_new_with_attributes("", cr, "text", 0, "font", 0, NULL));
  gtk_tree_view_append_column(fd->tv_name, GTK_TREE_VIEW_COLUMN(cl));
  gtk_container_add(GTK_CONTAINER(hbox), g_object_new(HILDON_TYPE_PANNABLE_AREA, "visible", TRUE, "child", fd->tv_name, NULL));
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(fd->tv_name), GTK_SELECTION_BROWSE);
  g_signal_connect(G_OBJECT(fd->tv_name), "realize", (GCallback)tv_realize, fd->dlg);

  fd->tm_size = GTK_TREE_MODEL(ls_size);
  fd->tv_size = g_object_new(GTK_TYPE_TREE_VIEW, "visible", TRUE, "hildon-ui-mode", HILDON_UI_MODE_EDIT, "model", ls_size, NULL);
  fd->sel_size = gtk_tree_view_get_selection(fd->tv_size);
  g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(fd->tv_size)), "changed", (GCallback)sel_changed, fd);
  pan = g_object_new(HILDON_TYPE_PANNABLE_AREA, "visible", TRUE, "child", fd->tv_size, NULL);
  cr = gtk_cell_renderer_text_new();
  gtk_tree_view_append_column(fd->tv_size, gtk_tree_view_column_new_with_attributes("", cr, "text", 0,  NULL));
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), pan, "expand", FALSE, NULL);
  gtk_widget_set_size_request(pan, 80, -1);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(fd->tv_size), GTK_SELECTION_BROWSE);
  g_signal_connect(G_OBJECT(fd->tv_size), "realize", (GCallback)tv_realize, fd->dlg);

  for (Nix = 0 ; Nix < G_N_ELEMENTS(font_sizes) ; Nix++) {
    str = g_strdup_printf("%d", font_sizes[Nix]);
    gtk_list_store_append(ls_size, &itr_size);
    gtk_list_store_set(ls_size, &itr_size,
      FONT_SIZE_STRING_COLUMN, str, FONT_SIZE_INT_COLUMN, font_sizes[Nix],
      -1);
    g_free(str);
  }

  gtk_widget_set_size_request(hbox, -1, 200);
  gtk_container_add(GTK_CONTAINER(fd->dlg->vbox), hbox);

  hbox = g_object_new(GTK_TYPE_HBOX, "visible", TRUE, "spacing", 8, "border-width", 8, NULL);
  str = g_strdup_printf("%s:", g_dgettext("gtk20", "Color"));
  gtk_container_add_with_properties(GTK_CONTAINER(hbox),
    g_object_new(GTK_TYPE_ALIGNMENT, "visible", TRUE, "xalign", 0.0, "yalign", 0.5, "xscale", 0.0, "yscale", 0.0, "child",
      g_object_new(GTK_TYPE_LABEL,
        "visible", TRUE, "use-underline", TRUE, "mnemonic-widget", fd->preview, "label", str, "justify", GTK_JUSTIFY_LEFT, NULL), NULL),
    "expand", FALSE, NULL);
  g_free(str);
  fd->fg_clr = g_object_new(HILDON_TYPE_COLOR_BUTTON, "visible", TRUE, NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), font_dialog.fg_clr, "expand", FALSE, NULL);
  g_signal_connect(G_OBJECT(fd->fg_clr), "notify::color", (GCallback)clr_changed, fd);
  fd->bg_clr = g_object_new(HILDON_TYPE_COLOR_BUTTON, "visible", TRUE, NULL);
  g_signal_connect(G_OBJECT(fd->bg_clr), "notify::color", (GCallback)clr_changed, fd);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), fd->bg_clr, "expand", FALSE, NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(font_dialog.dlg->vbox), hbox, "expand", FALSE, NULL);

  gtk_container_add_with_properties(GTK_CONTAINER(fd->dlg->vbox),
    g_object_new(GTK_TYPE_ALIGNMENT, "visible", TRUE, "xalign", 0.0, "yalign", 0.5, "xscale", 0.0, "yscale", 0.0, "border-width", 8, "child",
      g_object_new(GTK_TYPE_LABEL,
        "visible", TRUE, "use-underline", TRUE, "mnemonic-widget", fd->preview, "label", g_dgettext("gtk20", "_Preview:"), "justify", GTK_JUSTIFY_LEFT, NULL), NULL),
    "expand", FALSE, NULL);

  gtk_widget_set_size_request(GTK_WIDGET(fd->preview), -1, 92);
  gtk_container_add_with_properties(GTK_CONTAINER(fd->dlg->vbox), 
    g_object_new(GTK_TYPE_FRAME, "visible", TRUE, "shadow-type", GTK_SHADOW_NONE, "border-width", 8, "child", GTK_WIDGET(fd->preview), NULL), "expand", FALSE, NULL);
}

void
show_font_dialog(GtkWindow *parent)
{
  PangoFontDescription *pfd_face = NULL;
  PangoContext *pctx = gdk_pango_context_get();
  PangoFontFamily **families = NULL;
  PangoFontFace **faces = NULL;
  int n_families = 0, Nix, n_faces = 0, Nix1, size = 0;
  char *str = NULL, *name = NULL;
  GtkListStore *ls_name;
  GtkTreeIter itr_family;
  GConfClient *g_c = gconf_client_get_default();
  GdkColor clr, *p_clr;

  /* Create the font dialog if not already done */
  if (NULL == font_dialog.dlg)
    create_font_dialog(&font_dialog);
  gtk_window_set_transient_for(GTK_WINDOW(font_dialog.dlg), parent);
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(font_dialog.tv_name)))
    hildon_pannable_area_jump_to(HILDON_PANNABLE_AREA(gtk_widget_get_parent(GTK_WIDGET(font_dialog.tv_name))), -1, 0);
  if (GTK_WIDGET_REALIZED(GTK_WIDGET(font_dialog.tv_size)))
    hildon_pannable_area_jump_to(HILDON_PANNABLE_AREA(gtk_widget_get_parent(GTK_WIDGET(font_dialog.tv_size))), -1, 0);

  /* Re-fill the font list */
  ls_name = GTK_LIST_STORE(font_dialog.tm_name);
  gtk_list_store_clear(ls_name);
  pango_context_list_families(pctx, &families, &n_families);
  for (Nix = 0 ; Nix < n_families ; Nix++)
    if (pango_font_family_is_monospace(families[Nix])) {
      pango_font_family_list_faces(families[Nix], &faces, &n_faces);
      for (Nix1 = 0 ; Nix1 < n_faces ; Nix1++) {
        pfd_face = pango_font_face_describe(faces[Nix1]);
        str = pango_font_description_to_string(pfd_face);
        gtk_list_store_append(ls_name, &itr_family);
        gtk_list_store_set(ls_name, &itr_family, 
          FONT_NAME_STRING_COLUMN, str,         FONT_NAME_FAMILY_COLUMN, families[Nix], 
          FONT_NAME_FACE_COLUMN,   faces[Nix1], FONT_NAME_PFD_COLUMN,    pfd_face,
          -1);
        g_free(str);
      }
      g_free(faces); faces = NULL;
      n_faces = 0;
    }
  g_free(families);

  /* Init dialog from gconf */
  /* Font name */
  name = gconf_client_get_string(g_c, OSSO_XTERM_GCONF_FONT_NAME, NULL);
  if (!name)
    name = g_strdup(OSSO_XTERM_DEFAULT_FONT_NAME);

  /* Font size */
  size = gconf_client_get_int(g_c, OSSO_XTERM_GCONF_FONT_BASE_SIZE, NULL);
  if (!size)
    size = OSSO_XTERM_DEFAULT_FONT_BASE_SIZE;
  if (name && size) 
    select_font(name, size, &font_dialog);
  g_free(name); name = NULL;
  size = 0;

  /* Foreground colour */
  str = gconf_client_get_string(g_c, OSSO_XTERM_GCONF_FONT_COLOR, NULL);
  if (!str)
    str = g_strdup(OSSO_XTERM_DEFAULT_FONT_COLOR);
  gdk_color_parse(str, &clr);
  g_free(str);
  g_object_set(G_OBJECT(font_dialog.fg_clr), "color", &clr, NULL);

  /* Background colour */
  str = gconf_client_get_string(g_c, OSSO_XTERM_GCONF_BG_COLOR, NULL);
  if (!str)
    str = g_strdup(OSSO_XTERM_DEFAULT_BG_COLOR);
  gdk_color_parse(str, &clr);
  g_free(str);
  g_object_set(G_OBJECT(font_dialog.bg_clr), "color", &clr, NULL);

  if (GTK_RESPONSE_OK == gtk_dialog_run(font_dialog.dlg)) {
    GtkTreeIter itr_name, itr_size;

    /* Set new font name and size */
    if (gtk_tree_selection_get_selected(font_dialog.sel_name, NULL, &itr_name) &&
        gtk_tree_selection_get_selected(font_dialog.sel_size, NULL, &itr_size)) {

      gtk_tree_model_get(font_dialog.tm_name, &itr_name, FONT_NAME_STRING_COLUMN, &name, -1);
      gtk_tree_model_get(font_dialog.tm_size, &itr_size, FONT_SIZE_INT_COLUMN, &size, -1);

      if (name) {
        gconf_client_set_string(g_c, OSSO_XTERM_GCONF_FONT_NAME, name, NULL);
        g_free(name); name = NULL;
      }
      if (size)
        gconf_client_set_int(g_c, OSSO_XTERM_GCONF_FONT_BASE_SIZE, size, NULL);
    }

    /* Set foreground colour */
    g_object_get(G_OBJECT(font_dialog.fg_clr), "color", &p_clr, NULL);
    str = g_strdup_printf("#%02x%02x%02x", p_clr->red >> 8, p_clr->green >> 8, p_clr->blue >> 8);
    gconf_client_set_string(g_c, OSSO_XTERM_GCONF_FONT_COLOR, str, NULL);
    g_free(str);
    gdk_color_free(p_clr);

    /* Set background colour */
    g_object_get(G_OBJECT(font_dialog.bg_clr), "color", &p_clr, NULL);
    str = g_strdup_printf("#%02x%02x%02x", p_clr->red >> 8, p_clr->green >> 8, p_clr->blue >> 8);
    gconf_client_set_string(g_c, OSSO_XTERM_GCONF_BG_COLOR, str, NULL);
    g_free(str);
    gdk_color_free(p_clr);
  }
  gtk_widget_hide(GTK_WIDGET(font_dialog.dlg));
}
