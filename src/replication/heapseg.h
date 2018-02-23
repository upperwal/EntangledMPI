#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "src/shared.h"
#include "src/misc/log.h"

#ifndef __HEAP_SEG_H__
#define __HEAP_SEG_H__

typedef struct {
	address container_address;
	address linked_address;
	address allocated_address;	// irrelevant on replica node.
	size_t size;
} Malloc_container;


/*
* Doubly linkedlist to store memory allocation using malloc. This structure is maintained 
* to help migrate heap segment to the replica node. 
*
* New malloc requests are added to the tail of the linkedlist. When replication is initiated
* this list is read starting from tail (as it contains most recent entries) and only one 
* entry per location is send to the replica.
* Eg: int *a = malloc(13);
*          a = malloc(20);
* Allocation in 'a' twice should remove first allocation as it is  obsolete and only consider
* second allocation. (ALthrough user should free 'a' first and the use 'a' for new memory, but
* we know they wont do it).   
*/
typedef struct Malloc_list {
	Malloc_container container;
	struct Malloc_list *next;
	struct Malloc_list *prev;
} Malloc_list;

typedef enum Memory_state { CONTIGUOUS, DISCONTIGUOUS } Memory_state;

void rep_append(Malloc_container);
void rep_remove(void *);
void rep_display();
void rep_clear();
void rep_clear_discontiguous();

void assign_malloc_context(const void **, const void **);

void rep_malloc(void **, size_t);
void rep_free(void **);

int transfer_heap_seg(MPI_Comm);

#endif
