noinst_PROGRAMS = 	rep_test 				\
					data_seg_trans_test 	\
					stack_seg_trans_test 	\
					heap_seg_trans_test 	\
					checkpoint_test 		\
					mat_mul 				\
					scatter_test 			\
					gather_test 			\
					bcast_test 				\
					allgather_test 			\
					reduce_test 			\
					allreduce_test 			\
					ulfm_test 				\
					rep_collective_test 	\
					isend_irecv_test

rep_test_LDADD = libreplication.la 
rep_test_SOURCES = test/rep_test.c
rep_test_LDFLAGS = -no-install
rep_test_CFLAGS = $(AM_CFLAGS)

data_seg_trans_test_LDADD = libreplication.la
data_seg_trans_test_SOURCES = test/dataseg_transfer_test.c
data_seg_trans_test_LDFLAGS = -no-install
data_seg_trans_test_CFLAGS = $(AM_CFLAGS)

stack_seg_trans_test_LDADD = libreplication.la
stack_seg_trans_test_SOURCES = test/stackseg_transfer_test.c
stack_seg_trans_test_LDFLAGS = -no-install
stack_seg_trans_test_CFLAGS = $(AM_CFLAGS)

heap_seg_trans_test_LDADD = libreplication.la
heap_seg_trans_test_SOURCES = test/heapseg_transfer_test.c
heap_seg_trans_test_LDFLAGS = -no-install
heap_seg_trans_test_CFLAGS = $(AM_CFLAGS)

checkpoint_test_LDADD = libreplication.la
checkpoint_test_SOURCES = test/checkpoint_test.c
checkpoint_test_LDFLAGS = -no-install
checkpoint_test_CFLAGS = $(AM_CFLAGS)

mat_mul_LDADD = libreplication.la
mat_mul_SOURCES = test/mat_mul.c
mat_mul_LDFLAGS = -no-install
mat_mul_CFLAGS = $(AM_CFLAGS)

scatter_test_LDADD = libreplication.la
scatter_test_SOURCES = test/scatter_test.c
scatter_test_LDFLAGS = -no-install
scatter_test_CFLAGS = $(AM_CFLAGS)

gather_test_LDADD = libreplication.la
gather_test_SOURCES = test/gather_test.c
gather_test_LDFLAGS = -no-install
gather_test_CFLAGS = $(AM_CFLAGS)

bcast_test_LDADD = libreplication.la
bcast_test_SOURCES = test/bcast_test.c
bcast_test_LDFLAGS = -no-install
bcast_test_CFLAGS = $(AM_CFLAGS)

allgather_test_LDADD = libreplication.la
allgather_test_SOURCES = test/allgather_test.c
allgather_test_LDFLAGS = -no-install
allgather_test_CFLAGS = $(AM_CFLAGS)

reduce_test_LDADD = libreplication.la
reduce_test_SOURCES = test/reduce_test.c
reduce_test_LDFLAGS = -no-install
reduce_test_CFLAGS = $(AM_CFLAGS)

allreduce_test_LDADD = libreplication.la
allreduce_test_SOURCES = test/allreduce_test.c
allreduce_test_LDFLAGS = -no-install
allreduce_test_CFLAGS = $(AM_CFLAGS)

ulfm_test_LDADD = libreplication.la
ulfm_test_SOURCES = test/ulfm_test.c
ulfm_test_LDFLAGS = -no-install
ulfm_test_CFLAGS = $(AM_CFLAGS)

rep_collective_test_LDADD = libreplication.la
rep_collective_test_SOURCES = test/rep_collective_test.c
rep_collective_test_LDFLAGS = -no-install
rep_collective_test_CFLAGS = $(AM_CFLAGS)

isend_irecv_test_LDADD = libreplication.la
isend_irecv_test_SOURCES = test/isend_irecv_test.c
isend_irecv_test_LDFLAGS = -no-install
isend_irecv_test_CFLAGS = $(AM_CFLAGS)