MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = osso-xterm.service.in osso-xterm.desktop.in

dbusservicedir = $(datadir)/dbus-1/services
nodist_dbusservice_DATA = osso-xterm.service

#dist_hildondesktopentrydir = $(hildondesktopentrydir)
hildondesktopentrydir = $(datadir)/applications/hildon
nodist_hildondesktopentry_DATA = osso-xterm.desktop

gconfschemasdir = $(sysconfdir)/gconf/schemas
dist_gconfschemas_DATA = osso-xterm.schemas

edit = sed -e 's,@bindir\@,$(bindir),g'

%.service: %.service.in Makefile
	rm -f $@ $@.tmp
	$(edit) $< > $@.tmp
	mv $@.tmp $@

%.desktop: %.desktop.in Makefile
	rm -f $@ $@.tmp
	$(edit) $< > $@.tmp
	mv $@.tmp $@

distclean-local:
	$(RM) $(nodist_hildondesktopentry_DATA)
	$(RM) $(nodist_dbusservice_DATA)
