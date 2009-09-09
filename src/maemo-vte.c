#include <hildon/hildon.h>
#include <gdk/gdkkeysyms.h>
#include "maemo-vte.h"
#include "vte-marshallers.h"

typedef struct
{
  guint keyval;
  GdkModifierType mods;
} Keystroke;

enum
{
  PAN_MODE_PROPERTY = 1,
  CONTROL_MASK_PROPERTY,
  MATCH_PROPERTY,
};

static GdkPixbuf *fs_button_pb = NULL;

struct _MaemoVtePrivate
{
  GtkIMContext *imc;
  GtkAdjustment *foreign_vadj;
  gboolean pan_mode;
  gboolean control_mask;
  gboolean been_panning;
  char *match;
  guint fs_button_timeout_id;
  guint fadeout_anim_timeout_id;
  guint count;
  GdkPixbuf *next_frame;
  int cx_next_frame;
  int cy_next_frame;
  gboolean fs_button_click;
};

#define PERFORM_SYNC(mvte,src) \
  ((mvte)->priv->foreign_vadj && \
   (((mvte)->priv->pan_mode) || \
    ((src) != (mvte)->priv->foreign_vadj)))

enum {
  MVTE_FS_BUTTON_CLICKED_SIGNAL,
  MVTE_N_SIGNALS
};

static int mvte_signals[MVTE_N_SIGNALS] = { 0 };

static void set_control_mask(MaemoVte *mvte, gboolean on);

static void
set_up_sync(MaemoVte *mvte, GtkAdjustment **p_src, GtkAdjustment **p_dst, double *p_factor)
{
  if (!PERFORM_SYNC(mvte,(*p_src)))
    (*p_src) = vte_terminal_get_adjustment(VTE_TERMINAL(mvte));

  if ((*p_src) == mvte->priv->foreign_vadj) {
    (*p_dst) = vte_terminal_get_adjustment(VTE_TERMINAL(mvte));
    (*p_factor) = 1.0 / ((double)(VTE_TERMINAL(mvte)->char_height));
  }
  else {
    (*p_dst) = mvte->priv->foreign_vadj;
    (*p_factor) = ((double)(VTE_TERMINAL(mvte)->char_height));
  }
}

static void
sync_vadj(GtkAdjustment *src, MaemoVte *mvte)
{
  GtkAdjustment *dst;
  double factor;

  set_up_sync(mvte, &src, &dst, &factor);

  if (!(src && dst)) return;

  if (!(dst->upper          == src->upper * factor && 
        dst->lower          == src->lower * factor && 
        dst->step_increment == src->step_increment * factor && 
        dst->page_increment == src->page_increment * factor && 
        dst->page_size      == src->page_size * factor)) {
    dst->upper          = src->upper * factor;
    dst->lower          = src->lower * factor;
    dst->step_increment = src->step_increment * factor;
    dst->page_increment = src->page_increment * factor;
    dst->page_size      = src->page_size * factor;
    mvte->priv->been_panning = TRUE;
    gtk_adjustment_changed(dst);
  }
}

static void
sync_vadj_value(GtkAdjustment *src, MaemoVte *mvte)
{
  GtkAdjustment *dst;
  double factor;

  set_up_sync(mvte, &src, &dst, &factor);

  if (!(src && dst)) return;

  if (dst->value != src->value * factor) {
    dst->value = src->value * factor;
    mvte->priv->been_panning = TRUE;
    gtk_adjustment_value_changed(dst);
  }
}

static void
set_scroll_adjustments(MaemoVte *mvte, GtkAdjustment *hadjustment, GtkAdjustment *vadjustment)
{
  GtkAdjustment *my_vadj = vte_terminal_get_adjustment(VTE_TERMINAL(mvte));
  /* This function ignores hadjustment */

  if (mvte->priv->foreign_vadj) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(mvte->priv->foreign_vadj), sync_vadj,       mvte);
    g_signal_handlers_disconnect_by_func(G_OBJECT(mvte->priv->foreign_vadj), sync_vadj_value, mvte);
    g_signal_handlers_disconnect_by_func(G_OBJECT(my_vadj),                  sync_vadj,       mvte);
    g_signal_handlers_disconnect_by_func(G_OBJECT(my_vadj),                  sync_vadj_value, mvte);
    g_object_unref(mvte->priv->foreign_vadj);
    mvte->priv->foreign_vadj = NULL;
  }

  if (vadjustment) {
    mvte->priv->foreign_vadj = g_object_ref(vadjustment);
    g_signal_connect(G_OBJECT(vadjustment), "changed",       (GCallback)sync_vadj,       mvte);
    g_signal_connect(G_OBJECT(vadjustment), "value-changed", (GCallback)sync_vadj_value, mvte);
    g_signal_connect(G_OBJECT(my_vadj),     "changed",       (GCallback)sync_vadj,       mvte);
    g_signal_connect(G_OBJECT(my_vadj),     "value-changed", (GCallback)sync_vadj_value, mvte);
  }
}

static void
set_pan_mode(MaemoVte *mvte, gboolean pan_mode)
{
  if (pan_mode != mvte->priv->pan_mode) {
    mvte->priv->pan_mode = pan_mode;
    if (!pan_mode) {
      sync_vadj(vte_terminal_get_adjustment(VTE_TERMINAL(mvte)), mvte);
      sync_vadj_value(vte_terminal_get_adjustment(VTE_TERMINAL(mvte)), mvte);
    }
    g_object_notify(G_OBJECT(mvte), "pan-mode");
  }
}

static void
control_mode_commit(GtkIMContext *imc, gchar *text, GdkWindow *wnd)
{
  /* This function is soooo not Unicode safe ! */
  MaemoVte *mvte = g_object_get_data(G_OBJECT(wnd), "mvte");

  if (mvte)
    if (mvte->priv->control_mask) {
      int Nix;

      if (text)
        if (text[0] != 0) {
          GdkModifierType mod = 0;
          GdkEventKey *key_event = (GdkEventKey *)gdk_event_new(GDK_KEY_PRESS);

          if (key_event) {
            gdk_window_get_pointer(GTK_WIDGET(mvte)->window, NULL, NULL, &mod);

            /* Things that need to be unset after use and before freeing */
            key_event->window = GTK_WIDGET(mvte)->window;
            /* The rest are shallow things */
            key_event->time = GDK_CURRENT_TIME;
            key_event->state = (mod | GDK_CONTROL_MASK) & (~GDK_RELEASE_MASK);

            for (Nix = 0 ; text[Nix] != 0 ; Nix++) {
              key_event->keyval = text[Nix];

              key_event->type = GDK_KEY_PRESS;
              key_event->state &= (~GDK_RELEASE_MASK);

              gdk_event_put(((GdkEvent *)key_event));

              key_event->type = GDK_KEY_RELEASE;
              key_event->state |= GDK_RELEASE_MASK;

              gdk_event_put(((GdkEvent *)key_event));
            }

            /* Unset things that need to be unset after use and before freeing */
            key_event->window = NULL;
            key_event->string = NULL;
            key_event->length = 0;
            gdk_event_free((GdkEvent *)key_event);

            g_signal_stop_emission_by_name((gpointer)imc, "commit");
            set_control_mask(mvte, FALSE);
          }
        }
    }
}

static void
set_control_mask(MaemoVte *mvte, gboolean new_value)
{
  if (mvte->priv->control_mask != new_value) {
    mvte->priv->control_mask = new_value;
    g_object_notify(G_OBJECT(mvte), "control-mask");
  }
}

static void
set_property(GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PAN_MODE_PROPERTY:
      set_pan_mode(MAEMO_VTE(obj), g_value_get_boolean(value));
      break;

    case CONTROL_MASK_PROPERTY:
      set_control_mask(MAEMO_VTE(obj), g_value_get_boolean(value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
  }
}

static void
get_property(GObject *obj, guint property_id, GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PAN_MODE_PROPERTY:
      g_value_set_boolean(value, MAEMO_VTE(obj)->priv->pan_mode);
      break;

    case CONTROL_MASK_PROPERTY:
      g_value_set_boolean(value, MAEMO_VTE(obj)->priv->control_mask);
      break;

    case MATCH_PROPERTY:
      g_value_set_string(value, MAEMO_VTE(obj)->priv->match);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
  }
}

static gboolean
maybe_ignore_mouse_event(GtkWidget *widget, GdkEvent *event, gboolean (*mouse_event)(GtkWidget *, GdkEvent *))
{
  MaemoVte *mvte = MAEMO_VTE(widget);
  return mvte->priv->pan_mode                || 
         mvte->priv->fs_button_timeout_id    ||
         mvte->priv->fs_button_click
    ? TRUE
    : mouse_event
      ? mouse_event(widget, event)
      : FALSE;
}

static void
check_match(MaemoVte *mvte, int x, int y)
{
  VteTerminal *vte = VTE_TERMINAL(mvte);
  char *possible_match = NULL;
  int x_pad, y_pad;
  int tag = 0;

  vte_terminal_get_padding(vte, &x_pad, &y_pad);

  possible_match = vte_terminal_match_check(vte, (x - x_pad) / vte->char_width, (y - y_pad) / vte->char_height, &tag);

  if (possible_match || g_strcmp0(possible_match, mvte->priv->match)) {
    g_free(mvte->priv->match);
    mvte->priv->match = possible_match;
    g_object_notify(G_OBJECT(mvte), "match");
  }
}

static void
queue_fs_button_draw_update(MaemoVte *vs) {
  GdkDrawable *wnd = GTK_WIDGET(vs)->window;

  if (wnd && vs->priv->next_frame) {
    int cx, cy;
    GdkRectangle rc;

    gdk_drawable_get_size(wnd, &cx, &cy);
    rc.x      = cx - vs->priv->cx_next_frame;
    rc.y      = cy - vs->priv->cy_next_frame;
    rc.width  = vs->priv->cx_next_frame;
    rc.height = vs->priv->cy_next_frame;

    gdk_window_invalidate_rect(wnd, &rc, FALSE);
  }
}

void
maemo_vte_hide_fullscreen_button(MaemoVte *vs)
{
  if (vs->priv->fadeout_anim_timeout_id) {
    g_source_remove(vs->priv->fadeout_anim_timeout_id);
    vs->priv->fadeout_anim_timeout_id = 0;
  }
  if (vs->priv->fs_button_timeout_id) {
    g_source_remove(vs->priv->fs_button_timeout_id);
    vs->priv->fs_button_timeout_id = 0;
  }
  vs->priv->count = 0;

  queue_fs_button_draw_update(vs);
}

static gboolean
button_press_event(GtkWidget *widget, GdkEventButton *event)
{
  gboolean keep_processing = TRUE;
  MaemoVte *mvte = MAEMO_VTE(widget);

  if (mvte->priv->fs_button_timeout_id) {
    int cx, cy;

    gdk_drawable_get_size(widget->window, &cx, &cy);
    if (event->x >= cx - mvte->priv->cx_next_frame && event->x <= cx && 
        event->y >= cy - mvte->priv->cy_next_frame && event->y <= cy) {
      g_signal_emit(G_OBJECT(widget), mvte_signals[MVTE_FS_BUTTON_CLICKED_SIGNAL], 0);
      mvte->priv->fs_button_click = TRUE;
      keep_processing = FALSE;
    }
    maemo_vte_hide_fullscreen_button(mvte);
  }

  if (keep_processing) {
    gtk_widget_grab_focus(widget);

    mvte->priv->been_panning = FALSE;

    if (mvte->priv->imc)
      if (hildon_gtk_im_context_filter_event(mvte->priv->imc, (GdkEvent *)event))
        return TRUE;
  }

  return maybe_ignore_mouse_event (widget, (GdkEvent *)event,
    (gboolean (*)(GtkWidget *, GdkEvent *))
      (GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->button_press_event));
}

static gboolean
motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
  return maybe_ignore_mouse_event (widget, (GdkEvent *)event,
    (gboolean (*)(GtkWidget *, GdkEvent *))
      (GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->motion_notify_event));
}

static gboolean
button_release_event(GtkWidget *widget, GdkEventButton *event)
{
  MaemoVte *mvte = MAEMO_VTE(widget);

  if (mvte->priv->fs_button_click)
    mvte->priv->fs_button_click = FALSE;
  else 
  if (!(mvte->priv->fs_button_timeout_id)) {
    if (mvte->priv->imc)
      if (hildon_gtk_im_context_filter_event(mvte->priv->imc, (GdkEvent *)event))
        return TRUE;

    if (!(mvte->priv->been_panning))
      check_match(mvte, event->x, event->y);
  }

  return maybe_ignore_mouse_event (widget, (GdkEvent *)event,
    (gboolean (*)(GtkWidget *, GdkEvent *))
      (GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->button_release_event));
}

static gboolean
key_press_release_event(GtkWidget *widget, GdkEventKey *event)
{
  MaemoVte *mvte = MAEMO_VTE(widget);
  gboolean (*parent_key_press_release_event)(GtkWidget *, GdkEventKey *) =
    (event->type == GDK_KEY_PRESS)
      ? GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->key_press_event
      : GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->key_release_event;

  if (mvte->priv->control_mask)
    event->state |= GDK_CONTROL_MASK;

//  dump_key_event(event);

  return parent_key_press_release_event
    ? parent_key_press_release_event(widget, event)
    : FALSE;
}

static void (*orig_set_client_window)(GtkIMContext *imc, GdkWindow *window) = NULL;

static void
my_set_client_window(GtkIMContext *imc, GdkWindow *window)
{
  if (window) {
    g_object_set_data(G_OBJECT(window), "im-context", imc);
    g_signal_connect(G_OBJECT(imc), "commit", (GCallback)control_mode_commit, window);
  }

  if (orig_set_client_window)
    orig_set_client_window(imc, window);
}

static void
imc_retrieve_surrounding(GtkIMContext *imc, MaemoVte *mvte)
{
  gtk_im_context_set_surrounding(imc, "", -1, 0);
}

static void
realize(GtkWidget *widget)
{
  GtkIMContext *imc = NULL;
  void (*parent_realize)(GtkWidget *widget) = GTK_WIDGET_CLASS(g_type_class_peek(g_type_parent(MAEMO_VTE_TYPE)))->realize;

  if (parent_realize)
    parent_realize(widget);

  if (widget->window) {
    g_object_set_data(G_OBJECT(widget->window), "mvte", widget);
    imc = g_object_get_data(G_OBJECT(widget->window), "im-context");
    g_signal_connect(G_OBJECT(imc), "retrieve-surrounding", (GCallback)imc_retrieve_surrounding, widget);
    if (imc) {
      g_object_set(G_OBJECT(imc), "hildon-input-mode", HILDON_GTK_INPUT_MODE_FULL, NULL);
      MAEMO_VTE(widget)->priv->imc = imc;
    }
  }
}

static void
finalize(GObject *obj)
{
  void (*parent_finalize)(GObject *) = G_OBJECT_CLASS(g_type_class_peek(g_type_parent(MAEMO_VTE_TYPE)))->finalize;
  MaemoVte *mvte = MAEMO_VTE(obj);

  g_free(mvte->priv->match);
  if (parent_finalize)
    parent_finalize(obj);
}

static void
settings_notify(GObject *settings, GParamSpec *pspec, gpointer null)
{
  if (fs_button_pb)
    g_object_unref(fs_button_pb);

  fs_button_pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "general_fullsize", HILDON_ICON_PIXEL_SIZE_THUMB, 0, NULL);
}

static gboolean
expose_event(GtkWidget *widget, GdkEventExpose *event)
{
  MaemoVte *mvte = MAEMO_VTE(widget);
  GdkGC *gc;
  gboolean ret = FALSE;
  gboolean (*parent_expose_event) (GtkWidget *, GdkEventExpose *) =
    GTK_WIDGET_CLASS(g_type_class_peek(g_type_parent(MAEMO_VTE_TYPE)))->expose_event;

  if (parent_expose_event)
    ret = parent_expose_event(widget, event);

  if (mvte->priv->fs_button_timeout_id || mvte->priv->fadeout_anim_timeout_id) {
    gc = gdk_gc_new(widget->window);
    if (widget->window && gc) {
      int cx, cy;

      gdk_drawable_get_size(widget->window, &cx, &cy);
      gdk_gc_set_clip_region(gc, event->region);
      gdk_draw_pixbuf(widget->window, gc, mvte->priv->next_frame, 
        0, 0, 
        cx - mvte->priv->cx_next_frame, cy - mvte->priv->cy_next_frame,
        mvte->priv->cx_next_frame, mvte->priv->cy_next_frame, 
        GDK_RGB_DITHER_NONE, 0, 0);
      g_object_unref(gc);
    }
  }

  return ret;
}

static void
class_init(gpointer g_class, gpointer null)
{
  GtkIMContextClass *gtk_imc_class = GTK_IM_CONTEXT_CLASS(g_type_class_ref(GTK_TYPE_IM_MULTICONTEXT));
  GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
  MaemoVteClass *mvte_class = MAEMO_VTE_CLASS(g_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(g_class);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  /* Replace the set_client_window function for the GtkIMMultiContext class so we can trap when
     VteTerminal creates its im_context and make it available to MaemoVte through g_object_set_data */
  if (gtk_imc_class) {
    orig_set_client_window = gtk_imc_class->set_client_window;
    gtk_imc_class->set_client_window = my_set_client_window;
  }

  g_object_class_install_property(gobject_class, PAN_MODE_PROPERTY,
    g_param_spec_boolean("pan-mode", "Pan mode", "Toggle panning mode during which mouse events are ignored.",
      FALSE, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, CONTROL_MASK_PROPERTY,
    g_param_spec_boolean("control-mask", "Control Mask", "Controls whether text typed is treated as though each character were CTRL-modified",
      FALSE, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, MATCH_PROPERTY,
    g_param_spec_string("match", "Match", "The latest regex match the user has clicked on",
      NULL, G_PARAM_READABLE));

  widget_class->button_press_event = button_press_event;
  widget_class->motion_notify_event = motion_notify_event;
  widget_class->button_release_event = button_release_event;
  widget_class->key_press_event = key_press_release_event;
  widget_class->key_release_event = key_press_release_event;
  widget_class->realize = realize;
  widget_class->expose_event = expose_event;

  widget_class->set_scroll_adjustments_signal =
    g_signal_new(
      "set-scroll-adjustments",
      G_OBJECT_CLASS_TYPE(g_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET(MaemoVteClass, set_scroll_adjustments),
      NULL, NULL,
      _vte_marshal_VOID__OBJECT_OBJECT,
      G_TYPE_NONE, 2,
      GTK_TYPE_ADJUSTMENT,
      GTK_TYPE_ADJUSTMENT);

  mvte_signals[MVTE_FS_BUTTON_CLICKED_SIGNAL] =
    g_signal_new("fs-button-clicked",
      G_OBJECT_CLASS_TYPE(g_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET(MaemoVteClass, fs_button_clicked),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  mvte_class->set_scroll_adjustments = set_scroll_adjustments;

  g_signal_connect(gtk_settings_get_default(), "notify", (GCallback)settings_notify, NULL);
  settings_notify(NULL, NULL, NULL);

  g_type_class_add_private(g_class, sizeof(MaemoVtePrivate));
}

static void
instance_init(GTypeInstance *instance, gpointer g_class)
{
  MaemoVte *mvte = MAEMO_VTE(instance);
  GtkAdjustment *adj = NULL;

  mvte->priv = G_TYPE_INSTANCE_GET_PRIVATE(instance, MAEMO_VTE_TYPE, MaemoVtePrivate);
  mvte->priv->foreign_vadj = NULL;
  mvte->priv->pan_mode = FALSE;
  mvte->priv->control_mask = FALSE;
  mvte->priv->match = NULL;

  mvte->priv->fs_button_timeout_id = 0;
  mvte->priv->fadeout_anim_timeout_id = 0;
  mvte->priv->count = 0;
  mvte->priv->next_frame = NULL;
  mvte->priv->cx_next_frame = 0;
  mvte->priv->cy_next_frame = 0;
  mvte->priv->fs_button_click = FALSE;

  if ((adj = vte_terminal_get_adjustment(VTE_TERMINAL(instance))) != NULL) {
    g_signal_connect(G_OBJECT(adj), "changed",       (GCallback)sync_vadj,       instance);
    g_signal_connect(G_OBJECT(adj), "value-changed", (GCallback)sync_vadj_value, instance);
  }
}

GType
maemo_vte_get_type( void )
{
  static GType the_type = 0;

  if (!the_type) {
    static GTypeInfo the_type_info = {
      .class_size     = sizeof(MaemoVteClass),
      .base_init      = NULL,
      .base_finalize  = NULL,
      .class_init     = class_init,
      .class_finalize = NULL,
      .class_data     = NULL,
      .instance_size  = sizeof(MaemoVte),
      .n_preallocs    = 0,
      .instance_init  = instance_init,
      .value_table    = NULL
    };

    the_type = g_type_register_static(VTE_TYPE_TERMINAL, MAEMO_VTE_TYPE_STRING, &the_type_info, 0);
  }

  return the_type;
}

static int alphas[] = {63, 28, 15, 10, 7, 5, 3, 3, 2, 2, 1, 0};

static gboolean
fadeout_anim_timeout(MaemoVte *vs)
{
  if (vs->priv->count >= G_N_ELEMENTS(alphas)) {
    vs->priv->fadeout_anim_timeout_id = 0;
    return FALSE;
  }

  gdk_pixbuf_fill(vs->priv->next_frame, alphas[vs->priv->count]);
  gdk_pixbuf_composite(fs_button_pb, vs->priv->next_frame, 
    0, 0, 
    vs->priv->cx_next_frame, vs->priv->cy_next_frame, 
    0, 0,
    1.0, 1.0,
    GDK_INTERP_NEAREST,
    alphas[vs->priv->count]);

  vs->priv->count++;

  queue_fs_button_draw_update(vs);

  return TRUE;
}

static gboolean
fs_button_timeout(MaemoVte *vs)
{
  vs->priv->fs_button_timeout_id = 0;

  vs->priv->count = 0;
  vs->priv->fadeout_anim_timeout_id =
    g_timeout_add(100, (GSourceFunc)fadeout_anim_timeout, vs);

  return FALSE;
}

void
maemo_vte_show_fullscreen_button(MaemoVte *vs)
{
  if (GTK_WIDGET(vs)->window)
    gdk_window_process_updates(GTK_WIDGET(vs)->window, FALSE);

  maemo_vte_hide_fullscreen_button(vs);

  if (vs->priv->next_frame) {
    g_object_unref(vs->priv->next_frame);
    vs->priv->next_frame = NULL;
  }

  if (fs_button_pb) {
    vs->priv->next_frame = gdk_pixbuf_copy(fs_button_pb);

    vs->priv->cx_next_frame = gdk_pixbuf_get_width(fs_button_pb);
    vs->priv->cy_next_frame = gdk_pixbuf_get_height(fs_button_pb);

    gdk_pixbuf_fill(vs->priv->next_frame, 0x00000080);
    gdk_pixbuf_composite(fs_button_pb, vs->priv->next_frame, 
      0, 0, 
      vs->priv->cx_next_frame, vs->priv->cy_next_frame, 
      0, 0,
      1.0, 1.0,
      GDK_INTERP_NEAREST,
      255);
  }

  vs->priv->fs_button_timeout_id = 
    g_timeout_add(2000, (GSourceFunc)fs_button_timeout, vs);
}
