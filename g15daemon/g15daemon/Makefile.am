
METASOURCES = AUTO
sbin_PROGRAMS = g15daemon
noinst_PROGRAMS = g15daemontest
noinst_HEADERS = g15logo.h
g15daemon_SOURCES = g15_net.c g15_uinput.c logo.h utility_funcs.c \
	g15daemon.h main.c linked_lists.c gfx_primitives.c
g15daemontest_SOURCES = lcdclient_test.c
AM_CPPFLAGS = -I$(top_builddir)/libg15daemon_client/
g15daemontest_LDADD = $(top_builddir)/libg15daemon_client/libg15daemon_client.la

