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

struct _MaemoVtePrivate
{
  GtkIMContext *imc;
  GtkAdjustment *foreign_vadj;
  gboolean pan_mode;
  gboolean control_mask;
  gboolean been_panning;
  char *match;
};

#define PERFORM_SYNC(mvte,src) \
  ((mvte)->priv->foreign_vadj && \
   (((mvte)->priv->pan_mode) || \
    ((src) != (mvte)->priv->foreign_vadj)))

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
#if (0)
static void
dump_key_event(GdkEventKey *event)
{
  g_print(".type = %s\n.send_event = %s\n.time = %d\n.state = ", 
    event->type == GDK_KEY_PRESS   ? "GDK_KEY_PRESS"   : 
    event->type == GDK_KEY_RELEASE ? "GDK_KEY_RELEASE" : "Other",
    event->send_event ? "TRUE" : "FALSE", 
    event->time);

  if (event->state & GDK_SHIFT_MASK) g_print("GDK_SHIFT_MASK ");
  if (event->state & GDK_LOCK_MASK) g_print("GDK_LOCK_MASK ");
  if (event->state & GDK_CONTROL_MASK) g_print("GDK_CONTROL_MASK ");
  if (event->state & GDK_MOD1_MASK) g_print("GDK_MOD1_MASK ");
  if (event->state & GDK_MOD2_MASK) g_print("GDK_MOD2_MASK ");
  if (event->state & GDK_MOD3_MASK) g_print("GDK_MOD3_MASK ");
  if (event->state & GDK_MOD4_MASK) g_print("GDK_MOD4_MASK ");
  if (event->state & GDK_MOD5_MASK) g_print("GDK_MOD5_MASK ");
  if (event->state & GDK_BUTTON1_MASK) g_print("GDK_BUTTON1_MASK ");
  if (event->state & GDK_BUTTON2_MASK) g_print("GDK_BUTTON2_MASK ");
  if (event->state & GDK_BUTTON3_MASK) g_print("GDK_BUTTON3_MASK ");
  if (event->state & GDK_BUTTON4_MASK) g_print("GDK_BUTTON4_MASK ");
  if (event->state & GDK_BUTTON5_MASK) g_print("GDK_BUTTON5_MASK ");

  /* The next few modifiers are used by XKB, so we skip to the end.
   * Bits 15 - 25 are currently unused. Bit 29 is used internally.
   */
  
  if (event->state & GDK_SUPER_MASK) g_print("GDK_SUPER_MASK ");
  if (event->state & GDK_HYPER_MASK) g_print("GDK_HYPER_MASK ");
  if (event->state & GDK_META_MASK) g_print("GDK_META_MASK ");
  if (event->state & GDK_RELEASE_MASK) g_print("GDK_RELEASE_MASK ");
  if (event->state & GDK_MODIFIER_MASK) g_print("GDK_MODIFIER_MASK ");

  g_print("\n.keyval = %s\n.length = %d\n.string = %s\n.hardware_keycode = %d\n.group = %d\n.is_modifier = %s\n\n", 
    gdk_keyval_name(event->keyval), 
    event->length, 
    event->string,
    event->hardware_keycode,
    event->group,
    event->is_modifier ? "TRUE" : "FALSE");
}
#endif /* (0) */
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
  return MAEMO_VTE(widget)->priv->pan_mode
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

static gboolean
button_press_event(GtkWidget *widget, GdkEventButton *event)
{
  MaemoVte *mvte = MAEMO_VTE(widget);

  gtk_widget_grab_focus(widget);

  mvte->priv->been_panning = FALSE;

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

  if (!(mvte->priv->been_panning))
    check_match(mvte, event->x, event->y);

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
  mvte->priv->control_mask = FALSE;
  mvte->priv->match = NULL;
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
