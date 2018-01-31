noinst_PROGRAMS = reptest datasegtranstest stacksegtranstest
reptest_LDADD = libreplication.la
reptest_SOURCES = test/rep_test.c
reptest_LDFLAGS = -no-install
reptest_CFLAGS = -g -O0 -static

datasegtranstest_LDADD = libreplication.la
datasegtranstest_SOURCES = test/dataseg_transfer_test.c
datasegtranstest_LDFLAGS = -no-install
datasegtranstest_CFLAGS = -g -O0 -static

stacksegtranstest_LDADD = libreplication.la
stacksegtranstest_SOURCES = test/stackseg_transfer_test.c
stacksegtranstest_LDFLAGS = -no-install
stacksegtranstest_CFLAGS = -g -O0 -static