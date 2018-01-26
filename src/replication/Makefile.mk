lib_LTLIBRARIES += libreplication.la

libreplication_la_SOURCES = src/replication/rep.c 		\
							src/replication/rep.h 		\
							src/replication/dataseg.c	\
							src/replication/dataseg.h	\
							src/replication/stackseg.c	\
							src/replication/stackseg.h