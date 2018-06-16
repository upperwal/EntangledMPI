lib_LTLIBRARIES += libreplication.la
libreplication_la_SOURCES =  
libreplication_la_CFLAGS = $(REP_CFLAGS) $(LIB_FLAGS) -shared

noinst_HEADERS = src/shared.h

include src/manager/Makefile.mk
include src/mpi/Makefile.mk
include src/replication/Makefile.mk
include src/misc/Makefile.mk
include src/checkpoint/Makefile.mk
include src/fortran/Makefile.mk
