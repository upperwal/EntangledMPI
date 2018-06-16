LIB_FLAGS = -g -O3
AM_CFLAGS = $(REP_CFLAGS) $(LIB_FLAGS) -fpic

export GOPATH = @abs_srcdir@

include_HEADERS =
bin_PROGRAMS =
lib_LTLIBRARIES =

include include/Makefile.mk
include src/Makefile.mk
include test/Makefile.mk