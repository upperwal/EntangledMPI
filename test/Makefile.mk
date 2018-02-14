noinst_PROGRAMS = reptest datasegtranstest stacksegtranstest heapsegtranstest checkpointtest
reptest_LDADD = libreplication.la 
reptest_SOURCES = test/rep_test.c
reptest_LDFLAGS = -no-install
reptest_CFLAGS = -g -O0 -dynamic

datasegtranstest_LDADD = libreplication.la
datasegtranstest_SOURCES = test/dataseg_transfer_test.c
datasegtranstest_LDFLAGS = -no-install
datasegtranstest_CFLAGS = -g -O0 -dynamic

stacksegtranstest_LDADD = libreplication.la
stacksegtranstest_SOURCES = test/stackseg_transfer_test.c
stacksegtranstest_LDFLAGS = -no-install
stacksegtranstest_CFLAGS = -g -O0 -dynamic

heapsegtranstest_LDADD = libreplication.la
heapsegtranstest_SOURCES = test/heapseg_transfer_test.c
heapsegtranstest_LDFLAGS = -no-install
heapsegtranstest_CFLAGS = -g -O0 -dynamic

checkpointtest_LDADD = libreplication.la
checkpointtest_SOURCES = test/checkpoint_test.c
checkpointtest_LDFLAGS = -no-install
checkpointtest_CFLAGS = -g -O0 -dynamic