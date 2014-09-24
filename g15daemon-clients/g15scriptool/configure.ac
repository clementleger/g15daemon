AC_INIT(g15scriptool,[0.1svn], [mlampard@users.sf.net])
 
AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])
 
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_RANLIB

AC_CHECK_LIB([g15daemon_client], [g15daemon_version],,AC_MSG_ERROR(["libg15daemon_client (or its devel package) not found. please install it"]))
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(g15daemon_client.h libg15render.h,,AC_MSG_ERROR(["libg15daemon_client (or its devel package) could not be found.  please install it"]))

AM_PATH_GLIB_2_0(2.0.0,,)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.0.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
CPPFLAGS=$CPPFLAGS_save

AC_OUTPUT(Makefile)
