#ifndef _MAEMO_VTE_H_
#define _MAEMO_VTE_H_

#include <vte/vte.h>

G_BEGIN_DECLS

typedef struct _MaemoVte        MaemoVte;
typedef struct _MaemoVteClass   MaemoVteClass;
typedef struct _MaemoVtePrivate MaemoVtePrivate;

struct _MaemoVte
{
  VteTerminal parent_instance;

  MaemoVtePrivate *priv;
};

struct _MaemoVteClass
{
  VteTerminalClass parent_class;

  void (*set_scroll_adjustments) (MaemoVte *vs, GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);
};

GType maemo_vte_get_type( void );

#define MAEMO_VTE_TYPE_STRING "MaemoVte"
#define MAEMO_VTE_TYPE (maemo_vte_get_type())
#define MAEMO_VTE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), MAEMO_VTE_TYPE, MaemoVte))
#define MAEMO_VTE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), MAEMO_VTE_TYPE, MaemoVteClass))
#define MAEMO_VTE_PARENT_CLASS (g_type_class_peek(g_type_parent(MAEMO_VTE_TYPE)))

G_END_DECLS

#endif /* !_MAEMO_VTE_H_ */
