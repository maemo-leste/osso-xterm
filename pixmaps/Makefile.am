pixmapdir = $(datadir)/@PACKAGE@/pixmaps

if HAVE_SVG
  pixmap_DATA = $(wildcard *.svg)
else
  pixmap_DATA = $(wildcard *.png)
endif

EXTRA_DIST = $(pixmap_DATA)
