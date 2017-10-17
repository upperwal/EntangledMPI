AM_CPPFLAGS = -I$(top_builddir)/src/replication
bin_PROGRAMS += hello
hello_SOURCES = main.c $(top_builddir)/src/replication/funprint.h $(top_builddir)/src/replication/funprint.c