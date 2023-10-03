#include <string.h>
#include <stdlib.h>
#include <vte/vte.h>
#include <gconf/gconf-client.h>
#include <hildon/hildon.h>
#include "font-dialog.h"
#include "terminal-gconf.h"

#define PREVIEW_TEXT "drwxr-xr-x 2 user users"

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
  GtkWidget *preview;
  GtkWidget *preview_bg;

  GtkTreeView *tv_size;
  GtkTreeModel *tm_size;
  GtkTreeSelection *sel_size;

  GtkTreeView *tv_name;
  GtkTreeModel *tm_name;
  GtkTreeSelection *sel_name;

  GtkWidget *fg_clr;
  GtkWidget *bg_clr;

  GtkWidget *reverse_button;
  GtkWidget *scroll_button;
  GtkWidget *use_hw_keys_button;
  GtkWidget *scrollback_entry;
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

static int
compare_faces(PangoFontFace **p_face1, PangoFontFace **p_face2, gpointer null)
{
  /* most sig. ... weight ... style ... stretch ... variant ... least sig. */

  PangoFontDescription *pfd1 = pango_font_face_describe((*p_face1)), *pfd2 = pango_font_face_describe((*p_face2));

  int weight1, style1, stretch1, variant1,
      weight2, style2, stretch2, variant2;

  weight1  = pango_font_description_get_weight(pfd1);
  style1   = pango_font_description_get_style(pfd1);
  stretch1 = pango_font_description_get_stretch(pfd1);
  variant1 = pango_font_description_get_variant(pfd1);

  pango_font_description_free(pfd1);

  weight2  = pango_font_description_get_weight(pfd2);
  style2   = pango_font_description_get_style(pfd2);
  stretch2 = pango_font_description_get_stretch(pfd2);
  variant2 = pango_font_description_get_variant(pfd2);

  pango_font_description_free(pfd2);

  return
    (weight1 != weight2)
      ? ((weight1 < weight2) ? -1 : 1)
      : (style1 != style2)
        ? ((style1 < style2) ? -1 : 1)
        : (stretch1 != stretch2)
          ? ((stretch1 < stretch2) ? -1 : 1)
          : (variant1 != variant2)
            ? ((variant1 < variant2) ? -1 : 1)
            : 0;
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
      gtk_widget_modify_font(fd->preview, pfd);
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
  gtk_widget_modify_bg(GTK_WIDGET(fd->preview_bg), GTK_STATE_NORMAL, clr_bg);
  gtk_widget_modify_base(GTK_WIDGET(fd->preview_bg), GTK_STATE_NORMAL, clr_bg);
  gtk_widget_modify_fg(GTK_WIDGET(fd->preview), GTK_STATE_NORMAL, clr_fg);
  gdk_color_free(clr_fg);
  gdk_color_free(clr_bg);
}

static void
preview_realize(GtkWidget *preview, FontDialog *fd)
{
  g_object_notify(G_OBJECT(fd->fg_clr), "color");
}

static void
font_dialog_response(GtkWidget *dlg, gint response_id, FontDialog *fd)
{
  if (GTK_RESPONSE_OK == response_id) {
    char *name = NULL, *str = NULL;
    int size;
    gboolean b;
    gint lines;
    GtkTreeIter itr_name, itr_size;
    GConfClient *g_c = gconf_client_get_default();
    GdkColor *p_clr = NULL;

    /* Set new font name and size */
    if (gtk_tree_selection_get_selected(fd->sel_name, NULL, &itr_name) &&
        gtk_tree_selection_get_selected(fd->sel_size, NULL, &itr_size)) {

      gtk_tree_model_get(fd->tm_name, &itr_name, FONT_NAME_STRING_COLUMN, &name, -1);
      gtk_tree_model_get(fd->tm_size, &itr_size, FONT_SIZE_INT_COLUMN, &size, -1);

      if (name) {
        gconf_client_set_string(g_c, OSSO_XTERM_GCONF_FONT_NAME, name, NULL);
        g_free(name); name = NULL;
      }
      if (size)
        gconf_client_set_int(g_c, OSSO_XTERM_GCONF_FONT_BASE_SIZE, size, NULL);
    }

    /* Set foreground colour */
    g_object_get(G_OBJECT(fd->fg_clr), "color", &p_clr, NULL);
    str = g_strdup_printf("#%02x%02x%02x", p_clr->red >> 8, p_clr->green >> 8, p_clr->blue >> 8);
    gconf_client_set_string(g_c, OSSO_XTERM_GCONF_FONT_COLOR, str, NULL);
    g_free(str);
    gdk_color_free(p_clr);

    /* Set background colour */
    g_object_get(G_OBJECT(fd->bg_clr), "color", &p_clr, NULL);
    str = g_strdup_printf("#%02x%02x%02x", p_clr->red >> 8, p_clr->green >> 8, p_clr->blue >> 8);
    gconf_client_set_string(g_c, OSSO_XTERM_GCONF_BG_COLOR, str, NULL);
    g_free(str);
    gdk_color_free(p_clr);

    /* Set reverse color */
    b = hildon_check_button_get_active (HILDON_CHECK_BUTTON (fd->reverse_button));
    gconf_client_set_bool(g_c, OSSO_XTERM_GCONF_REVERSE, b, NULL);

    /* Set always scroll */
    b = hildon_check_button_get_active (HILDON_CHECK_BUTTON (fd->scroll_button));
    gconf_client_set_bool(g_c, OSSO_XTERM_GCONF_ALWAYS_SCROLL, b, NULL);

    /* Set use hw keys */
    b = hildon_check_button_get_active (HILDON_CHECK_BUTTON (fd->use_hw_keys_button));
    gconf_client_set_bool(g_c, OSSO_XTERM_GCONF_USE_HW_KEYS, b, NULL);

    /* Set scrollback lines */
    lines = atoi (gtk_entry_get_text (GTK_ENTRY (fd->scrollback_entry)));
    if (lines <= 0) lines = OSSO_XTERM_DEFAULT_SCROLLBACK;
    gconf_client_set_int(g_c, OSSO_XTERM_GCONF_SCROLLBACK, lines, NULL);
  }
  gtk_widget_destroy(GTK_WIDGET(fd->dlg));
  memset(fd, 0, sizeof(FontDialog));
}

static void
create_font_dialog(FontDialog *fd)
{
  int Nix;
  char *str;
  GtkTreeIter itr_size;
  GtkWidget
    *hbox = g_object_new(GTK_TYPE_HBOX, "visible", TRUE, "spacing", 8, NULL),
    *align,
    *pan;
  GtkListStore
    *ls_name = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_OBJECT, PANGO_TYPE_FONT_DESCRIPTION),
    *ls_size = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  GtkCellLayout *cl;
  GtkCellRenderer *cr;
  fd->dlg = GTK_DIALOG(gtk_dialog_new_with_buttons(g_dgettext("gtk20", "Preferences"), NULL, GTK_DIALOG_NO_SEPARATOR,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL));
  fd->preview = g_object_new(GTK_TYPE_LABEL, "visible", TRUE, "label", PREVIEW_TEXT, "justify", GTK_JUSTIFY_LEFT, "use-underline", FALSE, NULL);
  fd->preview_bg = g_object_new(GTK_TYPE_EVENT_BOX, "visible", TRUE, NULL);
  gtk_container_add(GTK_CONTAINER(fd->preview_bg),
    g_object_new(GTK_TYPE_ALIGNMENT, "visible", TRUE, "xalign", (double)0.0, "yalign", (double)0.0, "xscale", (double)0.0, "yscale", (double)0.0, "child", fd->preview, NULL));
  g_signal_connect(G_OBJECT(fd->preview), "realize", (GCallback)preview_realize, fd);

  fd->tm_name = GTK_TREE_MODEL(ls_name);
  fd->tv_name = g_object_new(GTK_TYPE_TREE_VIEW, "visible", TRUE, "hildon-ui-mode", HILDON_UI_MODE_EDIT, "model", ls_name, "enable-search", FALSE, NULL);
  fd->sel_name = gtk_tree_view_get_selection(fd->tv_name);
  g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(fd->tv_name)), "changed", (GCallback)sel_changed, fd);
  cr = gtk_cell_renderer_text_new();
  cl = GTK_CELL_LAYOUT(gtk_tree_view_column_new_with_attributes("", cr, "text", 0, "font", 0, NULL));
  gtk_tree_view_append_column(fd->tv_name, GTK_TREE_VIEW_COLUMN(cl));
  gtk_container_add(GTK_CONTAINER(hbox), g_object_new(HILDON_TYPE_PANNABLE_AREA, "visible", TRUE, "child", fd->tv_name, NULL));
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(fd->tv_name), GTK_SELECTION_BROWSE);
  g_signal_connect(G_OBJECT(fd->tv_name), "realize", (GCallback)tv_realize, fd->dlg);

  fd->tm_size = GTK_TREE_MODEL(ls_size);
  fd->tv_size = g_object_new(GTK_TYPE_TREE_VIEW, "visible", TRUE, "hildon-ui-mode", HILDON_UI_MODE_EDIT, "model", ls_size, "enable-search", FALSE, NULL);
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

  gtk_widget_set_size_request(hbox, -1, 205);
  gtk_container_add(GTK_CONTAINER(fd->dlg->vbox), hbox);

  hbox = g_object_new(GTK_TYPE_HBOX, "visible", TRUE, "spacing", 8, NULL);
  align = g_object_new(GTK_TYPE_ALIGNMENT, "visible", TRUE, "xalign", 0.0, "yalign", 0.5, "xscale", 1.0, "yscale", 0.0, "child", hbox, NULL);
  gtk_widget_set_size_request(align, -1, 70);
  fd->fg_clr = g_object_new(HILDON_TYPE_COLOR_BUTTON, "visible", TRUE, NULL);
  fd->bg_clr = g_object_new(HILDON_TYPE_COLOR_BUTTON, "visible", TRUE, NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox),
    g_object_new(GTK_TYPE_ALIGNMENT, "visible", TRUE, "xalign", 0.0, "yalign", 0.5, "xscale", 0.0, "yscale", 0.0, "child",
      g_object_new(GTK_TYPE_LABEL,
        "visible", TRUE, "use-underline", TRUE, "mnemonic-widget", fd->fg_clr, "label", g_dgettext("gtk20", "Color"), "justify", GTK_JUSTIFY_LEFT, NULL), NULL),
    "expand", FALSE, NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), fd->fg_clr, "expand", FALSE, NULL);
  g_signal_connect(G_OBJECT(fd->fg_clr), "notify::color", (GCallback)clr_changed, fd);
  g_signal_connect(G_OBJECT(fd->bg_clr), "notify::color", (GCallback)clr_changed, fd);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), fd->bg_clr, "expand", FALSE, NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(fd->dlg->vbox), align, "expand", FALSE, NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), fd->preview_bg, "expand", TRUE, NULL);
  g_signal_connect(G_OBJECT(fd->dlg), "response", (GCallback)font_dialog_response, fd);
  
  GConfClient *gconf_client = gconf_client_get_default ();
  gboolean reverse = OSSO_XTERM_DEFAULT_REVERSE;
  gboolean always_scroll = OSSO_XTERM_DEFAULT_ALWAYS_SCROLL;
  gboolean use_hw_keys = OSSO_XTERM_DEFAULT_USE_HW_KEYS;
  gint scrollback = OSSO_XTERM_DEFAULT_SCROLLBACK;
  GConfValue *value;
  
  value = gconf_client_get (gconf_client, OSSO_XTERM_GCONF_REVERSE, NULL);
  if (value && value->type == GCONF_VALUE_BOOL)
    reverse = gconf_value_get_bool (value);

  value = gconf_client_get (gconf_client, OSSO_XTERM_GCONF_ALWAYS_SCROLL, NULL);
  if (value && value->type == GCONF_VALUE_BOOL)
    always_scroll = gconf_value_get_bool (value);

  value = gconf_client_get (gconf_client, OSSO_XTERM_GCONF_USE_HW_KEYS, NULL);
  if (value && value->type == GCONF_VALUE_BOOL)
    use_hw_keys = gconf_value_get_bool (value);

  value = gconf_client_get (gconf_client, OSSO_XTERM_GCONF_SCROLLBACK, NULL);
  if (value && value->type == GCONF_VALUE_INT)
    scrollback = gconf_value_get_int (value);
  if (scrollback <= 0) scrollback = OSSO_XTERM_DEFAULT_SCROLLBACK;

  fd->reverse_button = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (fd->reverse_button), reverse);
  gtk_widget_set_size_request (fd->reverse_button, -1, 60);
  gtk_button_set_label (GTK_BUTTON (fd->reverse_button), "Reverse colors");
  
  fd->scroll_button = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (fd->scroll_button), always_scroll);
  gtk_widget_set_size_request (fd->scroll_button, -1, 60);
  gtk_button_set_label (GTK_BUTTON (fd->scroll_button), "Always scroll");

  fd->use_hw_keys_button = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (fd->use_hw_keys_button), use_hw_keys);
  gtk_widget_set_size_request (fd->use_hw_keys_button, -1, 60);
  gtk_button_set_label (GTK_BUTTON (fd->use_hw_keys_button), "HW keys");
  
  hbox = gtk_hbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (hbox), fd->reverse_button);
  gtk_container_add (GTK_CONTAINER (hbox), fd->scroll_button);
  gtk_container_add (GTK_CONTAINER (hbox), fd->use_hw_keys_button);
  gtk_widget_show_all (hbox);
  gtk_container_add (GTK_CONTAINER (fd->dlg->vbox), hbox);

  GtkWidget *scrollback_label = gtk_label_new ("Terminal scrollback lines:");
  fd->scrollback_entry = hildon_entry_new (HILDON_SIZE_AUTO);
  gchar* scrollback_s = g_strdup_printf ("%i", scrollback);
  gtk_entry_set_text (GTK_ENTRY (fd->scrollback_entry), scrollback_s);
  g_free(scrollback_s);
  gtk_widget_set_size_request (fd->scrollback_entry, -1, 50);
  g_object_set (G_OBJECT (fd->scrollback_entry), "hildon-input-mode", HILDON_GTK_INPUT_MODE_NUMERIC, NULL);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (hbox), scrollback_label);
  gtk_container_add (GTK_CONTAINER (hbox), fd->scrollback_entry);
  gtk_widget_show_all (hbox);
  gtk_container_add (GTK_CONTAINER (fd->dlg->vbox), hbox);
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
  GdkColor clr;

  /* Create the font dialog if not already done */
  if (NULL == font_dialog.dlg) {
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
        g_qsort_with_data(faces, n_faces, sizeof(PangoFontFace *), (GCompareDataFunc)compare_faces, NULL);
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

    gtk_widget_show(GTK_WIDGET(font_dialog.dlg));
  }
  else {
    gtk_widget_hide(GTK_WIDGET(font_dialog.dlg));
    gtk_window_set_transient_for(GTK_WINDOW(font_dialog.dlg), parent);
    gtk_window_present(GTK_WINDOW(font_dialog.dlg));
  }
}
