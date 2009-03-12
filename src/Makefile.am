MAINTAINERCLEANFILES = Makefile.in

INCLUDES =                      \
	-DDBUS_API_SUBJECT_TO_CHANGE  \
	-D_GNU_SOURCE                 \
	-DG_LOG_DOMAIN=\"osso-xterm\" \
  -DDATADIR="\"$(datadir)\""    \
	$(NULL)

bin_PROGRAMS = osso-xterm

osso_xterm_CFLAGS =     \
	$(VTE_CFLAGS)         \
	$(GCONF_CFLAGS)       \
	$(HILDON_LIBS_CFLAGS) \
	$(HILDON_CFLAGS)      \
	$(OSSO_CFLAGS)        \
	$(DBUS_CFLAGS)        \
	$(BROWSER_CFLAGS)     \
	$(LAUNCHER_CFLAGS)    \
  $(NULL)

osso_xterm_LDADD =    \
	$(DBUS_LIBS)        \
	$(GCONF_LIBS)       \
	$(HILDON_LIBS_LIBS) \
	$(HILDON_LIBS)      \
	$(OSSO_LIBS)        \
	$(VTE_LIBS)         \
  $(NULL)

EXTRA_DIST =           \
  vte-marshallers.list \
  $(NULL)

BUILT_SOURCES =     \
  stock-icon-list.h \
  vte-marshallers.c \
  vte-marshallers.h \
  $(NULL)

osso_xterm_LDFLAGS =  \
	-no-undefined       \
	$(LAUNCHER_LDFLAGS) \
  $(NULL)

osso_xterm_headers =    \
	terminal-gconf.h      \
	terminal-settings.h   \
	terminal-tab-header.h \
	terminal-widget.h     \
	terminal-window.h     \
	terminal-manager.h    \
	terminal-encoding.h   \
	shortcuts.h           \
  stock-icons.h         \
  $(NULL)

osso_xterm_SOURCES =    \
	$(osso_xterm_headers) \
	main.c                \
	maemo-vte.c           \
	terminal-tab-header.c \
	terminal-settings.c   \
	terminal-widget.c     \
	terminal-window.c     \
	terminal-manager.c    \
	terminal-encoding.c   \
	shortcuts.c           \
  stock-icons.c         \
  $(NULL)

nodist_osso_xterm_SOURCES = $(BUILT_SOURCES)

vte-marshallers.c: vte-marshallers.h vte-marshallers.list 
	( echo '#include "vte-marshallers.h"' ; @GLIB_GENMARSHAL@ --body --prefix=_vte_marshal vte-marshallers.list ; ) > $@.tmp
	mv $@.tmp $@

vte-marshallers.h: vte-marshallers.list
	@GLIB_GENMARSHAL@ --header --prefix=_vte_marshal vte-marshallers.list > $@.tmp
	mv $@.tmp $@

clean-local:
	$(RM) *.core core core.* stamp-*.h *~

if HAVE_SVG
ICON_EXTENSION = svg
else
ICON_EXTENSION = png
endif

stock-icon-list.h: $(shell ls ../pixmaps/*.$(ICON_EXTENSION))
	echo -n ''                                                                                                        > $@.tmp
	echo '#ifndef _HAVE_STOCK_ICON_LIST_H_'                                                                          >> $@.tmp
	echo '#define _HAVE_STOCK_ICON_LIST_H_'                                                                          >> $@.tmp
	echo 'static struct { const char *fname ; const char *stock_name ; } stock_icon_list[] = {'                      >> $@.tmp
	ls ../pixmaps | grep -v Makefile | grep '\.$(ICON_EXTENSION)' | while read ; do \
	  echo '{ .fname = "'$$REPLY'", .stock_name = "'@PACKAGE@-$$(echo $$REPLY | sed 's/\.$(ICON_EXTENSION)//')'" },' >> $@.tmp ; \
	done ;
	echo '};'                                                                                                        >> $@.tmp
	echo '#endif /* !_HAVE_STOCK_ICON_LIST_H_ */'                                                                    >> $@.tmp
	mv $@.tmp $@

CLEANFILES = $(BUILT_SOURCES) vte-marshallers.c.tmp vte-marshallers.h.tmp
