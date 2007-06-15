MAINTAINERCLEANFILES = Makefile.in

INCLUDES =								\
	-DDBUS_API_SUBJECT_TO_CHANGE					\
	-D_GNU_SOURCE 					 		\
	-DG_LOG_DOMAIN=\"osso-xterm\"

bin_PROGRAMS = osso-xterm

osso_xterm_CFLAGS =							\
	@VTE_CFLAGS@							\
	@GCONF_CFLAGS@							\
	@HILDON_LIBS_CFLAGS@						\
	@OSSO_CFLAGS@							\
	@DBUS_CFLAGS@							\
	$(LAUNCHER_CFLAGS)

osso_xterm_LDADD =							\
	@DBUS_LIBS@							\
	@GCONF_LIBS@							\
	@HILDON_LIBS_LIBS@						\
	@OSSO_LIBS@							\
	@VTE_LIBS@

osso_xterm_LDFLAGS =							\
	-no-undefined							\
	$(LAUNCHER_LDFLAGS)

osso_xterm_headers =							\
	terminal-gconf.h						\
	terminal-settings.h						\
	terminal-tab-header.h						\
	terminal-widget.h						\
	terminal-app.h

osso_xterm_SOURCES =							\
	$(osso_xterm_headers)						\
	main.c								\
	terminal-tab-header.c						\
	terminal-settings.c						\
	terminal-widget.c						\
	terminal-app.c

clean-local:
	$(RM) *.core core core.* stamp-*.h *~

