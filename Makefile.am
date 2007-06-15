MAINTAINERCLEANFILES = \
        Makefile.in config.h.in configure \
        install-sh ltmain.sh missing mkinstalldirs \
        compile depcomp config.log config.status \
        config.guess config.sub acinclude.m4 aclocal.m4 \
        build-stamp

SUBDIRS = icons etc src

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL NEWS README THANKS aclocal/ax_cflags_gcc_option.m4

distclean-local:
	$(RM) -r autom4te.cache
	$(RM) -r osso-xterm-*.tar.gz *~

