#include "stock-icons.h"
#include "stock-icon-list.h"

#define PIXMAP_DIR DATADIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "pixmaps"

static gchar *
find_pixmap_file (const gchar *filename)
{
  gchar *pathname;

  pathname = g_strdup_printf ("%s%s", PIXMAP_DIR G_DIR_SEPARATOR_S, filename);
  if (g_file_test (pathname, G_FILE_TEST_EXISTS)) return pathname;
  g_free (pathname);

  return NULL;
}

static void
add_stock_icon (const gchar *filename, const gchar *stock_id)
{
  GtkIconSource *icon_source = NULL ;
  GtkIconSet *icon_set = NULL ;
  static GtkIconFactory *icon_factory = NULL ;
  char *psz = NULL ;

  if (NULL == icon_factory)
    gtk_icon_factory_add_default (icon_factory = gtk_icon_factory_new ()) ;

  if (NULL != (icon_source = gtk_icon_source_new ())) {
    gtk_icon_source_set_filename (icon_source, psz = find_pixmap_file (filename)) ;
//    gtk_icon_source_set_size_wildcarded (icon_source, TRUE) ;
    if (NULL == (icon_set = gtk_icon_factory_lookup (icon_factory, stock_id))) {
      icon_set = gtk_icon_set_new () ;
      gtk_icon_factory_add (icon_factory, stock_id, icon_set) ;
    }
    gtk_icon_set_add_source (icon_set, icon_source) ;
  }
}

void
add_stock_icons( void )
{
  int Nix;

  for (Nix = G_N_ELEMENTS(stock_icon_list) - 1 ; Nix > -1 ; Nix--)
    add_stock_icon(stock_icon_list[Nix].fname, stock_icon_list[Nix].stock_name);
}
