AC_INIT([osso-xterm],[0.14.mh26])
dnl Initialize automake
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_INSTALL
AC_ISC_POSIX
AC_HEADER_STDC
AC_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.35.0])

AX_CFLAGS_GCC_OPTION(-std=c99)
AX_CFLAGS_GCC_OPTION(-Wall)
dnl AX_CFLAGS_GCC_OPTION(-Werror)
AX_CFLAGS_GCC_OPTION(-Wmissing-prototypes)
AX_CFLAGS_GCC_OPTION(-Wmissing-declarations)
AX_CFLAGS_GCC_OPTION(-Wunreachable-code)
AX_CFLAGS_GCC_OPTION(-Wwrite-strings)

AC_MSG_CHECKING([whether to use maemo launcher])
AC_ARG_ENABLE([maemo-launcher],
	      [AS_HELP_STRING([--enable-maemo-launcher],
	      [default=yes])],
	      [
	       case "${enableval}" in
	       yes) maemo_launcher=true
	            AC_MSG_RESULT([yes])
		    PKG_CHECK_MODULES(MAEMO_LAUNCHER, [maemo-launcher-app])
		    AC_SUBST(MAEMO_LAUNCHER_CFLAGS)
		    MAEMO_LAUNCHER_LIBS="-Xcompiler \"$MAEMO_LAUNCHER_LIBS\""
		    AC_SUBST(MAEMO_LAUNCHER_LIBS)
		    ;;
	       no)  maemo_launcher=false
                    AC_MSG_RESULT([no])
		    ;;
	       *) AC_MSG_ERROR([
			"Bad value ${enableval} for --enable-maemo-launcher,\n"
			"give exactly 'yes' or 'no' value."]) ;;
              esac
	     ], [maemo_launcher=false])

PKG_CHECK_MODULES(VTE, vte >= 0.11.0)
AC_SUBST(VTE_LIBS)
AC_SUBST(VTE_CFLAGS)

PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= 0.60)
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6.0)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

PKG_CHECK_MODULES(OSSO, libosso >= 0.7.5)
AC_SUBST(VTE_LIBS)
AC_SUBST(VTE_CFLAGS)

PKG_CHECK_MODULES(HILDON, hildon-1 >= 0.9.9, hildon1=yes, hildon1=no)
AC_DEFINE(HILDON, 1, [Version of hildon libraries])
	
AC_SUBST(HILDON_LIBS)
AC_SUBST(HILDON_CFLAGS)
AC_SUBST(HILDON_LIBS_LIBS)
AC_SUBST(HILDON_LIBS_CFLAGS)

AC_DEFINE_UNQUOTED(LOCALEDIR, "/usr/share/locale", Locale dir)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
if test "x$GLIB_GENMARSHAL" == "x"; then
  AC_ERROR([Can't find glib-genmarshal in path. Fix your installation.])
fi
AC_SUBST([GLIB_GENMARSHAL])

dnl ===========================================================================
dnl === gettext                                                             ===
dnl ===========================================================================

GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="cs da de es_ES es_MX fi fr_CA fr_FR it nl no pl pt ru sv"
AM_GLIB_GNU_GETTEXT

AC_OUTPUT([
Makefile
icons/Makefile
icons/24x24/Makefile
icons/25x25/Makefile
icons/48x48/Makefile
icons/64x64/Makefile
etc/Makefile
src/Makefile
po/Makefile.in
])
