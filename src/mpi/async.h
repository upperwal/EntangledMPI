#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#include "src/shared.h"
#include "src/misc/log.h"

#ifndef __ASYNC_H__
#define __ASYNC_H__

typedef struct Request_List {
	MPI_Request *req;
	struct Request_List *next;
} Request_List;

typedef struct {
	int request_count;
	Request_List *list;
} Aggregate_Request;

Aggregate_Request *new_agg_request();
MPI_Request *add_new_request(Aggregate_Request *);
int get_request_count(void *);
MPI_Request *get_array_of_request(void *, int *);
int free_agg_request(void *);
int wait_for_agg_request(void *, MPI_Status *);

#endif
