lib_LTLIBRARIES += libreplication.la
libreplication_la_SOURCES = 
libreplication_la_CFLAGS = -g -o0 -dynamic

include src/manager/Makefile.mk
include src/mpi/Makefile.mk
include src/replication/Makefile.mk
