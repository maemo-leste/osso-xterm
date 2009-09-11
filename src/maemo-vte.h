#ifndef _MAEMO_VTE_H_
#define _MAEMO_VTE_H_

#include <vte/vte.h>

G_BEGIN_DECLS

typedef struct _MaemoVte        MaemoVte;
typedef struct _MaemoVteClass   MaemoVteClass;

GType maemo_vte_get_type( void );

#define MAEMO_VTE_TYPE_STRING "MaemoVte"
#define MAEMO_VTE_TYPE (maemo_vte_get_type())
#define MAEMO_VTE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), MAEMO_VTE_TYPE, MaemoVte))
#define MAEMO_VTE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), MAEMO_VTE_TYPE, MaemoVteClass))

void maemo_vte_show_overlay_pixbuf(MaemoVte *vs);
void maemo_vte_hide_overlay_pixbuf(MaemoVte *vs);

G_END_DECLS

#endif /* !_MAEMO_VTE_H_ */
