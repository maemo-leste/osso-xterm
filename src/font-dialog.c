#include <string.h>
#include <stdlib.h>
#include <vte/vte.h>
#include <gconf/gconf-client.h>
#include <hildon/hildon.h>
#include "font-dialog.h"
#include "terminal-gconf.h"
#include "he-simple-color-dialog.h"

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

static const int
SCROLLBACK_LINES_OPTIONS[] = {
    200,
    5000,
    10000,
    100000,
    1000000,
};

typedef struct {
    const char *label;
    const char *fg;
    const char *bg;
    gboolean is_custom;
} ColorSchemeOption;

static ColorSchemeOption
COLOR_SCHEME_OPTIONS[] = {
    {
        .label = "Black on white",
        .fg = OSSO_XTERM_DEFAULT_FONT_COLOR,
        .bg = OSSO_XTERM_DEFAULT_BG_COLOR,
        .is_custom = FALSE,
    },
    {
        .label = "White on black",
        .fg = OSSO_XTERM_DEFAULT_BG_COLOR,
        .bg = OSSO_XTERM_DEFAULT_FONT_COLOR,
        .is_custom = FALSE,
    },
    {
        .label = "Grey on black",
        .fg = "#cccccc",
        .bg = OSSO_XTERM_DEFAULT_FONT_COLOR,
        .is_custom = FALSE,
    },
    {
        .label = "Green on black",
        .fg = "#00ee00",
        .bg = OSSO_XTERM_DEFAULT_FONT_COLOR,
        .is_custom = FALSE,
    },
    {
        .label = "Custom",
        .is_custom = TRUE,
    },
};

typedef struct
{
  GtkDialog *dlg;
  GtkDialog *font_dlg;
  HildonButton *font_button;
  GtkWidget *preview;
  GtkWidget *preview_bg;

  GtkTreeView *tv_size;
  GtkTreeModel *tm_size;
  GtkTreeSelection *sel_size;

  GtkTreeView *tv_name;
  GtkTreeModel *tm_name;
  GtkTreeSelection *sel_name;

  GtkWidget *colorscheme_chooser;
  HildonTouchSelector *colorscheme_selector;
  GtkWidget *colorscheme_custom;

  GdkColor custom_fg_color;
  GdkColor custom_bg_color;

  GdkColor active_fg_color;
  GdkColor active_bg_color;

  GtkWidget *scroll_button;
  GtkWidget *use_hw_keys_button;

  GtkWidget *fg_clr_button;
  GtkWidget *bg_clr_button;

  int custom_scrollback_lines;
  int scrollback_lines;
} FontDialog;

static FontDialog font_dialog = { NULL };

static const guint8 font_sizes[] = {6, 8, 10, 12, 16, 18, 20, 22, 24, 28, 30, 32};

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
  hildon_button_set_value(HILDON_BUTTON(fd->font_button), str);
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
  gchar *name = NULL;
  GtkTreeIter itr;
  int size;

  if (gtk_tree_selection_get_selected(fd->sel_size, NULL, &itr)) {
    gtk_tree_model_get(fd->tm_size, &itr, FONT_SIZE_INT_COLUMN, &size, -1);

    if (gtk_tree_selection_get_selected(fd->sel_name, NULL, &itr))
      gtk_tree_model_get(fd->tm_name, &itr,
              FONT_NAME_PFD_COLUMN, &pfd,
              FONT_NAME_STRING_COLUMN, &name,
              -1);
  }

  if (pfd != NULL && name != NULL && size > 0) {
    pango_font_description_set_size(pfd, size * PANGO_SCALE);
    gtk_widget_modify_font(fd->preview, pfd);
    gchar *str = g_strdup_printf("%s %d", name, size);
    hildon_button_set_value(HILDON_BUTTON(fd->font_button), str);
    free(str);
  }

  if (pfd != NULL) {
    pango_font_description_free(pfd);
  }
  if (name != NULL) {
    g_free(name);
  }
}

static void
update_preview_colors(FontDialog *fd)
{
  gtk_widget_modify_bg(GTK_WIDGET(fd->preview_bg), GTK_STATE_NORMAL, &fd->active_bg_color);
  gtk_widget_modify_base(GTK_WIDGET(fd->preview_bg), GTK_STATE_NORMAL, &fd->active_bg_color);
  gtk_widget_modify_fg(GTK_WIDGET(fd->preview), GTK_STATE_NORMAL, &fd->active_fg_color);

  gtk_button_set_image(GTK_BUTTON(fd->fg_clr_button),
      he_simple_color_dialog_helper_create_color_widget(&fd->active_fg_color));
  gtk_button_set_image(GTK_BUTTON(fd->bg_clr_button),
      he_simple_color_dialog_helper_create_color_widget(&fd->active_bg_color));
}

static void
edit_color_with_dialog(FontDialog *fd, GdkColor *dest)
{
  HeSimpleColorDialog *dlg = HE_SIMPLE_COLOR_DIALOG(he_simple_color_dialog_new());
  he_simple_color_dialog_set_color(dlg, dest);
  gtk_widget_show_all(GTK_WIDGET(dlg));
  if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
    GdkColor *result = he_simple_color_dialog_get_color(dlg);
    *dest = *result;
    gdk_color_free(result);
    update_preview_colors(fd);
  }
  gtk_widget_destroy(GTK_WIDGET(dlg));
}

static void
on_fg_clr_button_clicked(GtkWidget *button, FontDialog *fd)
{
  edit_color_with_dialog(fd, &fd->active_fg_color);
}

static void
on_bg_clr_button_clicked(GtkWidget *button, FontDialog *fd)
{
  edit_color_with_dialog(fd, &fd->active_bg_color);
}

static void
preview_realize(GtkWidget *preview, FontDialog *fd)
{
  update_preview_colors(fd);
}

static void
font_dialog_response(GtkWidget *dlg, gint response_id, FontDialog *fd)
{
  if (GTK_RESPONSE_OK == response_id) {
    char *name = NULL, *str = NULL;
    int size;
    gboolean b;
    GtkTreeIter itr_name, itr_size;
    GConfClient *g_c = gconf_client_get_default();

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
    str = g_strdup_printf("#%02x%02x%02x",
            fd->active_fg_color.red >> 8,
            fd->active_fg_color.green >> 8,
            fd->active_fg_color.blue >> 8);
    gconf_client_set_string(g_c, OSSO_XTERM_GCONF_FONT_COLOR, str, NULL);
    g_free(str);

    /* Set background colour */
    str = g_strdup_printf("#%02x%02x%02x",
            fd->active_bg_color.red >> 8,
            fd->active_bg_color.green >> 8,
            fd->active_bg_color.blue >> 8);
    gconf_client_set_string(g_c, OSSO_XTERM_GCONF_BG_COLOR, str, NULL);
    g_free(str);

    /* Set reverse color; for backwards compatibility, force reverse false */
    gconf_client_set_bool(g_c, OSSO_XTERM_GCONF_REVERSE, FALSE, NULL);

    /* Set always scroll */
    b = hildon_check_button_get_active (HILDON_CHECK_BUTTON (fd->scroll_button));
    gconf_client_set_bool(g_c, OSSO_XTERM_GCONF_ALWAYS_SCROLL, b, NULL);

    /* Set use hw keys */
    b = hildon_check_button_get_active (HILDON_CHECK_BUTTON (fd->use_hw_keys_button));
    gconf_client_set_bool(g_c, OSSO_XTERM_GCONF_USE_HW_KEYS, b, NULL);

    /* Set scrollback lines */
    gconf_client_set_int(g_c, OSSO_XTERM_GCONF_SCROLLBACK, fd->scrollback_lines, NULL);
  }
  gtk_widget_destroy(GTK_WIDGET(fd->dlg));
  memset(fd, 0, sizeof(FontDialog));
}

static void
run_font_chooser_dialog(GtkButton *button, FontDialog *fd)
{
    gtk_dialog_run(fd->font_dlg);
    gtk_widget_hide(GTK_WIDGET(fd->font_dlg));
}

static void
on_colorscheme_chooser_value_changed(HildonPickerButton *widget, FontDialog *fd)
{
    gint active = hildon_picker_button_get_active(widget);

    if (active < 0 || active >= G_N_ELEMENTS(COLOR_SCHEME_OPTIONS)) {
        return;
    }

    ColorSchemeOption *option = &COLOR_SCHEME_OPTIONS[active];

    GdkColor fg_color, bg_color;
    if (option->is_custom) {
        gtk_widget_show_all(fd->colorscheme_custom);
        fg_color = fd->custom_fg_color;
        bg_color = fd->custom_bg_color;
    } else {
        gtk_widget_hide(fd->colorscheme_custom);
        gtk_window_resize(GTK_WINDOW(fd->dlg), 1, 1);
        gdk_color_parse(option->fg, &fg_color);
        gdk_color_parse(option->bg, &bg_color);
    }

    fd->active_fg_color = fg_color;
    fd->active_bg_color = bg_color;
    update_preview_colors(fd);
}

static void
on_scrollback_lines_chooser_value_changed(HildonPickerButton *widget, FontDialog *fd)
{
    gint active = hildon_picker_button_get_active(widget);

    if (active >= 0 && active < G_N_ELEMENTS(SCROLLBACK_LINES_OPTIONS)) {
        fd->scrollback_lines = SCROLLBACK_LINES_OPTIONS[active];
    } else {
        fd->scrollback_lines = fd->custom_scrollback_lines;
    }
}

static void
create_font_dialog(FontDialog *fd)
{
  fd->dlg = GTK_DIALOG(gtk_dialog_new_with_buttons(g_dgettext("gtk20", "Preferences"), NULL, GTK_DIALOG_NO_SEPARATOR,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL));

  GtkWidget *pannable = hildon_pannable_area_new();
  gtk_widget_set_size_request(pannable, -1, 400);
  gtk_container_add(GTK_CONTAINER(fd->dlg->vbox), pannable);

  GtkVBox *pannable_vbox = GTK_VBOX(gtk_vbox_new(FALSE, 0));
  hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable), GTK_WIDGET(pannable_vbox));
  gtk_widget_show_all(pannable);

  fd->preview = g_object_new(GTK_TYPE_LABEL,
          "visible", TRUE,
          "label",
              "drwxr-xr-x 22 user user .\n"
              "drwxr-xr-x  3 root root ..",
          "justify", GTK_JUSTIFY_LEFT,
          "use-underline", FALSE,
          NULL);
  fd->preview_bg = g_object_new(GTK_TYPE_EVENT_BOX, "visible", TRUE, NULL);
  gtk_container_add(GTK_CONTAINER(fd->preview_bg),
          g_object_new(GTK_TYPE_ALIGNMENT,
              "visible", TRUE,
              "xalign", 0.0,
              "yalign", 0.0,
              "xscale", 0.0,
              "yscale", 0.0,
              "child", fd->preview,
              NULL));
  g_signal_connect(G_OBJECT(fd->preview), "realize", (GCallback)preview_realize, fd);
  gtk_widget_set_size_request(fd->preview_bg, -1, 80);
  gtk_container_add_with_properties(GTK_CONTAINER(pannable_vbox), fd->preview_bg,
          "expand", FALSE,
          NULL);

  fd->tm_name =
      GTK_TREE_MODEL(gtk_list_store_new(4,
                  G_TYPE_STRING,
                  G_TYPE_OBJECT,
                  G_TYPE_OBJECT,
                  PANGO_TYPE_FONT_DESCRIPTION));

  fd->tm_size =
      GTK_TREE_MODEL(gtk_list_store_new(2,
                  G_TYPE_STRING,
                  G_TYPE_INT));

  GtkWidget *hbox = g_object_new(GTK_TYPE_HBOX,
          "visible", TRUE,
          "spacing", 8,
          NULL);

  fd->font_dlg = GTK_DIALOG(gtk_dialog_new_with_buttons("Choose font", NULL, GTK_DIALOG_NO_SEPARATOR,
              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
              "Done", GTK_RESPONSE_OK,
              NULL));

  fd->tm_name = GTK_TREE_MODEL(fd->tm_name);
  fd->tv_name = g_object_new(GTK_TYPE_TREE_VIEW,
          "visible", TRUE,
          "hildon-ui-mode", HILDON_UI_MODE_EDIT,
          "model", fd->tm_name,
          "enable-search", FALSE,
          NULL);
  fd->sel_name = gtk_tree_view_get_selection(fd->tv_name);
  g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(fd->tv_name)), "changed", (GCallback)sel_changed, fd);

  GtkCellRenderer *cr = gtk_cell_renderer_text_new();
  GtkCellLayout *cl = GTK_CELL_LAYOUT(gtk_tree_view_column_new_with_attributes("", cr,
          "text", 0,
          "font", 0,
          NULL));
  gtk_tree_view_append_column(fd->tv_name, GTK_TREE_VIEW_COLUMN(cl));

  gtk_container_add(GTK_CONTAINER(hbox),
          g_object_new(HILDON_TYPE_PANNABLE_AREA,
              "visible", TRUE,
              "child", fd->tv_name,
              NULL));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(fd->tv_name), GTK_SELECTION_BROWSE);
  g_signal_connect(G_OBJECT(fd->tv_name), "realize", (GCallback)tv_realize, fd->font_dlg);

  fd->tv_size = g_object_new(GTK_TYPE_TREE_VIEW,
          "visible", TRUE,
          "hildon-ui-mode", HILDON_UI_MODE_EDIT,
          "model", fd->tm_size,
          "enable-search", FALSE,
          NULL);
  fd->sel_size = gtk_tree_view_get_selection(fd->tv_size);
  g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(fd->tv_size)), "changed", (GCallback)sel_changed, fd);

  cr = gtk_cell_renderer_text_new();
  gtk_tree_view_append_column(fd->tv_size, gtk_tree_view_column_new_with_attributes("", cr, "text", 0,  NULL));
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(fd->tv_size), GTK_SELECTION_BROWSE);
  g_signal_connect(G_OBJECT(fd->tv_size), "realize", (GCallback)tv_realize, fd->font_dlg);

  GtkWidget *pan = g_object_new(HILDON_TYPE_PANNABLE_AREA,
          "visible", TRUE,
          "child", fd->tv_size,
          NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), pan,
          "expand", TRUE,
          NULL);
  gtk_widget_set_size_request(pan, 220, -1);

  for (int Nix = 0 ; Nix < G_N_ELEMENTS(font_sizes) ; Nix++) {
      gchar *str = g_strdup_printf("%d", font_sizes[Nix]);
      GtkTreeIter itr_size;
      gtk_list_store_append(GTK_LIST_STORE(fd->tm_size), &itr_size);
      gtk_list_store_set(GTK_LIST_STORE(fd->tm_size), &itr_size,
              FONT_SIZE_STRING_COLUMN, str,
              FONT_SIZE_INT_COLUMN, font_sizes[Nix],
              -1);
      g_free(str);
  }

  gtk_widget_set_size_request(hbox, -1, 350);

  gtk_container_add(GTK_CONTAINER(fd->font_dlg->vbox), hbox);
  gtk_widget_show_all(hbox);

  fd->font_button = HILDON_BUTTON(hildon_button_new_with_text(HILDON_SIZE_FINGER_HEIGHT,
          HILDON_BUTTON_ARRANGEMENT_VERTICAL,
          "Font",
          NULL /* will be set "on show" */));

  g_signal_connect(fd->font_button, "clicked", G_CALLBACK(run_font_chooser_dialog), fd);
  gtk_button_set_alignment(GTK_BUTTON(fd->font_button), 0.f, 0.5f);
  hildon_button_set_style(HILDON_BUTTON(fd->font_button), HILDON_BUTTON_STYLE_PICKER);
  gtk_widget_show(GTK_WIDGET(fd->font_button));
  gtk_container_add_with_properties(GTK_CONTAINER(pannable_vbox), GTK_WIDGET(fd->font_button),
          "expand", TRUE,
          NULL);

  fd->colorscheme_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
  for (int i=0; i<G_N_ELEMENTS(COLOR_SCHEME_OPTIONS); ++i) {
      ColorSchemeOption *option = &COLOR_SCHEME_OPTIONS[i];
      hildon_touch_selector_append_text(fd->colorscheme_selector, option->label);
  }

  fd->colorscheme_chooser = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  g_signal_connect(fd->colorscheme_chooser, "value-changed", G_CALLBACK(on_colorscheme_chooser_value_changed), fd);
  hildon_button_set_title(HILDON_BUTTON(fd->colorscheme_chooser), "Color scheme");
  gtk_button_set_alignment(GTK_BUTTON(fd->colorscheme_chooser), 0.f, 0.5f);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(fd->colorscheme_chooser),
          HILDON_TOUCH_SELECTOR(fd->colorscheme_selector));
  gtk_widget_show_all(fd->colorscheme_chooser);
  gtk_container_add_with_properties(GTK_CONTAINER(pannable_vbox), fd->colorscheme_chooser, "expand", TRUE, NULL);

  hbox = g_object_new(GTK_TYPE_HBOX,
          "visible", TRUE,
          NULL);

  fd->colorscheme_custom = g_object_new(GTK_TYPE_ALIGNMENT,
          "visible", TRUE,
          "xalign", 0.0,
          "yalign", 0.5,
          "xscale", 1.0,
          "yscale", 0.0,
          "child", hbox,
          NULL);

  GtkWidget *spacer = g_object_new(GTK_TYPE_EVENT_BOX, "visible", TRUE, NULL);
  gtk_widget_set_size_request(spacer, 30, -1);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), spacer, "expand", FALSE, NULL);

  fd->fg_clr_button = hildon_gtk_button_new(HILDON_SIZE_FINGER_HEIGHT);
  g_object_set(fd->fg_clr_button,
      "label", "Text color",
      NULL);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), fd->fg_clr_button, "expand", FALSE, NULL);
  g_signal_connect(G_OBJECT(fd->fg_clr_button), "clicked", (GCallback)on_fg_clr_button_clicked, fd);


  fd->bg_clr_button = hildon_gtk_button_new(HILDON_SIZE_FINGER_HEIGHT);
  g_object_set(fd->bg_clr_button,
      "label", "Background color",
      NULL);
  g_signal_connect(G_OBJECT(fd->bg_clr_button), "clicked", (GCallback)on_bg_clr_button_clicked, fd);
  gtk_container_add_with_properties(GTK_CONTAINER(hbox), fd->bg_clr_button, "expand", FALSE, NULL);

  gtk_container_add_with_properties(GTK_CONTAINER(pannable_vbox), fd->colorscheme_custom,
          "expand", FALSE,
          NULL);

  g_signal_connect(G_OBJECT(fd->dlg), "response", (GCallback)font_dialog_response, fd);

  GConfClient *gconf_client = gconf_client_get_default ();
  gboolean always_scroll = OSSO_XTERM_DEFAULT_ALWAYS_SCROLL;
  gboolean use_hw_keys = OSSO_XTERM_DEFAULT_USE_HW_KEYS;
  gint scrollback = OSSO_XTERM_DEFAULT_SCROLLBACK;
  GConfValue *value;
  
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

  fd->scroll_button = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (fd->scroll_button), always_scroll);
  gtk_button_set_label (GTK_BUTTON (fd->scroll_button), "Scroll on output");

  fd->use_hw_keys_button = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (fd->use_hw_keys_button), use_hw_keys);
  gtk_button_set_label (GTK_BUTTON (fd->use_hw_keys_button), "Adjust font size using volume keys");
  
  gtk_container_add (GTK_CONTAINER (pannable_vbox), fd->use_hw_keys_button);
  gtk_widget_show_all(fd->use_hw_keys_button);
  gtk_container_add (GTK_CONTAINER (pannable_vbox), fd->scroll_button);
  gtk_widget_show_all(fd->scroll_button);

  HildonTouchSelector *scrollback_linesselector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
  gboolean found_scrollback = FALSE;
  for (int i=0; i<G_N_ELEMENTS(SCROLLBACK_LINES_OPTIONS); ++i) {
      gchar *tmp = g_strdup_printf("%'d lines", SCROLLBACK_LINES_OPTIONS[i]);
      hildon_touch_selector_append_text(scrollback_linesselector, tmp);
      g_free(tmp);

      if (scrollback == SCROLLBACK_LINES_OPTIONS[i]) {
          hildon_touch_selector_set_active(scrollback_linesselector, 0, i);
          found_scrollback = TRUE;
      }
  }

  if (!found_scrollback) {
      // Add a custom option for a non-preset scrollback value
      gchar *tmp = g_strdup_printf("%'d lines", scrollback);
      hildon_touch_selector_append_text(scrollback_linesselector, tmp);
      g_free(tmp);
      hildon_touch_selector_set_active(scrollback_linesselector, 0, G_N_ELEMENTS(SCROLLBACK_LINES_OPTIONS));
  }

  fd->custom_scrollback_lines = scrollback;
  fd->scrollback_lines = scrollback;

  GtkWidget *scrollback_lines_chooser = hildon_picker_button_new(
          HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  g_signal_connect(scrollback_lines_chooser, "value-changed", G_CALLBACK(on_scrollback_lines_chooser_value_changed), fd);
  hildon_button_set_title(HILDON_BUTTON(scrollback_lines_chooser), "Scrollback buffer");
  gtk_button_set_alignment(GTK_BUTTON(scrollback_lines_chooser), 0.f, 0.5f);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(scrollback_lines_chooser),
          HILDON_TOUCH_SELECTOR(scrollback_linesselector));
  gtk_widget_show_all(scrollback_lines_chooser);
  gtk_container_add_with_properties(GTK_CONTAINER(pannable_vbox), scrollback_lines_chooser, "expand", TRUE, NULL);
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
    GdkColor fg_color;
    str = gconf_client_get_string(g_c, OSSO_XTERM_GCONF_FONT_COLOR, NULL);
    if (!str)
      str = g_strdup(OSSO_XTERM_DEFAULT_FONT_COLOR);
    gdk_color_parse(str, &fg_color);
    g_free(str);

    /* Background colour */
    GdkColor bg_color;
    str = gconf_client_get_string(g_c, OSSO_XTERM_GCONF_BG_COLOR, NULL);
    if (!str)
      str = g_strdup(OSSO_XTERM_DEFAULT_BG_COLOR);
    gdk_color_parse(str, &bg_color);
    g_free(str);

    /* Reverse option (for backwards compatibility only) */
    GConfValue *value;
    gboolean reverse = OSSO_XTERM_DEFAULT_REVERSE;
    value = gconf_client_get (g_c, OSSO_XTERM_GCONF_REVERSE, NULL);
    if (value) {
        if (value->type == GCONF_VALUE_BOOL) {
            reverse = gconf_value_get_bool (value);
        }
        gconf_value_free(value);
    }

    if (reverse) {
        /**
         * Flip values around when we read them;
         * saving will always write reverse=false
         */
        GdkColor tmp = fg_color;
        fg_color = bg_color;
        bg_color = tmp;
    }

    font_dialog.active_fg_color = fg_color;
    font_dialog.active_bg_color = bg_color;

    font_dialog.custom_fg_color = fg_color;
    font_dialog.custom_bg_color = bg_color;

    for (int i=0; i<G_N_ELEMENTS(COLOR_SCHEME_OPTIONS); ++i) {
        ColorSchemeOption *option = &COLOR_SCHEME_OPTIONS[i];

        GdkColor colorscheme_fg, colorscheme_bg;
        gdk_color_parse(option->fg, &colorscheme_fg);
        gdk_color_parse(option->bg, &colorscheme_bg);

        gboolean match = (gdk_color_equal(&colorscheme_fg, &fg_color) &&
                          gdk_color_equal(&colorscheme_bg, &bg_color));

        if (match || option->is_custom) {
            hildon_touch_selector_set_active(font_dialog.colorscheme_selector, 0, i);
            break;
        }
    }

    gtk_widget_show(GTK_WIDGET(font_dialog.dlg));
  }
  else {
    gtk_widget_hide(GTK_WIDGET(font_dialog.dlg));
    gtk_window_set_transient_for(GTK_WINDOW(font_dialog.dlg), parent);
    gtk_window_present(GTK_WINDOW(font_dialog.dlg));
  }
}
