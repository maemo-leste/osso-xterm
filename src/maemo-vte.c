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
  PAN_MODE_PROPERTY = 1
};

struct _MaemoVtePrivate
{
  GtkIMContext *imc;
  GtkAdjustment *foreign_vadj;
  gboolean pan_mode;
};

#define PERFORM_SYNC(mvte,src) \
  ((mvte)->priv->foreign_vadj && \
   (((mvte)->priv->pan_mode) || \
    ((src) != (mvte)->priv->foreign_vadj)))

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
    gtk_adjustment_changed(dst);
  }
}

static void
sync_vadj_value(GtkAdjustment *src, MaemoVte *mvte)
{
  GtkAdjustment *dst;
  double factor;

  set_up_sync(mvte, &src, &dst, &factor);
  if (dst->value != src->value * factor) {
    dst->value = src->value * factor;
    gtk_adjustment_value_changed(dst);
  }
}

static void
set_scroll_adjustments (MaemoVte *mvte, GtkAdjustment *hadjustment, GtkAdjustment *vadjustment)
{
  /* This function ignores hadjustment */

  if (mvte->priv->foreign_vadj) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(mvte->priv->foreign_vadj), sync_vadj,       mvte);
    g_signal_handlers_disconnect_by_func(G_OBJECT(mvte->priv->foreign_vadj), sync_vadj_value, mvte);
    g_object_unref(mvte->priv->foreign_vadj);
    mvte->priv->foreign_vadj = NULL;
  }

  if (vadjustment) {
    mvte->priv->foreign_vadj = g_object_ref(vadjustment);
    g_signal_connect(G_OBJECT(vadjustment), "changed",       (GCallback)sync_vadj,       mvte);
    g_signal_connect(G_OBJECT(vadjustment), "value-changed", (GCallback)sync_vadj_value, mvte);
  }
}

static void
finalize(GObject *obj)
{
  MaemoVte *mvte = MAEMO_VTE(obj);

  set_scroll_adjustments(mvte, NULL, NULL);
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
set_property(GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PAN_MODE_PROPERTY:
      set_pan_mode(MAEMO_VTE(obj), g_value_get_boolean(value));
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
  }
}

static gboolean
maybe_ignore_mouse_event(GtkWidget *widget, GdkEvent *event, gboolean (*mouse_event)(GtkWidget *, GdkEvent *))
{
  return MAEMO_VTE(widget)->priv->pan_mode
    ? TRUE
    : mouse_event
      ? mouse_event(widget, event)
      : FALSE;
}

static gboolean
button_press_event(GtkWidget *widget, GdkEventButton *event)
{
  MaemoVte *mvte = MAEMO_VTE(widget);

  gtk_widget_grab_focus(widget);

  if (mvte->priv->imc)
    if (hildon_gtk_im_context_filter_event(mvte->priv->imc, (GdkEvent *)event))
      return TRUE;

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

  if (mvte->priv->imc)
    if (hildon_gtk_im_context_filter_event(mvte->priv->imc, (GdkEvent *)event))
      return TRUE;

  return maybe_ignore_mouse_event (widget, (GdkEvent *)event,
    (gboolean (*)(GtkWidget *, GdkEvent *))
      (GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->button_release_event));
}

static gboolean
key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  gboolean (*parent_key_press_event)(GtkWidget *, GdkEventKey *) =
    GTK_WIDGET_CLASS(MAEMO_VTE_PARENT_CLASS)->key_press_event;

  return parent_key_press_event
    ? parent_key_press_event(widget, event)
    : FALSE;
}

static void (*orig_set_client_window)(GtkIMContext *imc, GdkWindow *window) = NULL;

static void
my_set_client_window(GtkIMContext *imc, GdkWindow *window)
{
  if (window)
    g_object_set_data(G_OBJECT(window), "im-context", imc);

  if (orig_set_client_window)
    orig_set_client_window(imc, window);
}

static void
realize(GtkWidget *widget)
{
  GtkIMContext *imc = NULL;
  void (*parent_realize)(GtkWidget *widget) = GTK_WIDGET_CLASS(g_type_class_peek(g_type_parent(MAEMO_VTE_TYPE)))->realize;

  if (parent_realize)
    parent_realize(widget);

  if (widget->window) {
    imc = g_object_get_data(G_OBJECT(widget->window), "im-context");
    if (imc) {
      int input_mode = 0;

      g_object_get(G_OBJECT(imc), "hildon-input-mode", &input_mode, NULL);
      input_mode = (input_mode & (~HILDON_GTK_INPUT_MODE_AUTOCAP)) | HILDON_GTK_INPUT_MODE_MULTILINE;
      g_object_set(G_OBJECT(imc), "hildon-input-mode", input_mode, NULL);

      MAEMO_VTE(widget)->priv->imc = imc;
    }
  }
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

  widget_class->button_press_event = button_press_event;
  widget_class->motion_notify_event = motion_notify_event;
  widget_class->button_release_event = button_release_event;
  widget_class->key_press_event = key_press_event;
  widget_class->realize = realize;

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

  mvte_class->set_scroll_adjustments = set_scroll_adjustments;

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
  if ((adj = vte_terminal_get_adjustment(VTE_TERMINAL(instance))) != NULL) {
    g_signal_connect(G_OBJECT(adj), "changed", (GCallback)sync_vadj, instance);
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
