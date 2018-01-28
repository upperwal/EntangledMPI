lib_LTLIBRARIES += libreplication.la
libreplication_la_SOURCES = 

include src/manager/Makefile.mk
include src/mpi/Makefile.mk
include src/replication/Makefile.mk
