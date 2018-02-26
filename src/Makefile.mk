lib_LTLIBRARIES += libreplication.la
libreplication_la_SOURCES = 
libreplication_la_CFLAGS = $(AM_CFLAGS)

include src/manager/Makefile.mk
include src/mpi/Makefile.mk
include src/replication/Makefile.mk
include src/misc/Makefile.mk
include src/checkpoint/Makefile.mk
