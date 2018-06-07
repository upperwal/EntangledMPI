#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#include "src/shared.h"
#include "src/misc/log.h"

#ifndef __ASYNC_H__
#define __ASYNC_H__

typedef struct {
	MPI_Request *req;
	struct Request_List *next;
} Request_List;

typedef struct {
	void *buf;
	struct Buffer_List *next;
} Buffer_List;

typedef enum Async_Type { NONE, ANY_SEND, ANY_RECV, IRECV } Async_Type;

typedef struct {
	int request_count;
	Request_List *list;
	Buffer_List *buf_list;
	void *original_buffer;
	int buffer_size;
	Async_Type async_type;
	int tag;

} Aggregate_Request;

Aggregate_Request *new_agg_request(void *, MPI_Datatype, int);
MPI_Request *add_new_request_and_buffer(Aggregate_Request *, void **);
int get_request_count(void *);
MPI_Request *get_array_of_request(void *, int *);
int free_agg_request(void *);
int wait_for_agg_request(void *, MPI_Status *);

#endif
