#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#ifndef __COMM_H__
#define __COMM_H__

/* 
*  Structure to hold each node's new rank. This will work as a wrapper to 
*  existing comms 
*/
typedef struct Nodes {
	int job_id;
	int rank;
	int age;
} Node;

typedef struct Jobs {
	int job_id;
	int worker_count;
	int *rank_list;
} Job;

int init_nodes(char *, Job **, Node *);
int parse_map_file(char *, Job **, Node *);

#endif
