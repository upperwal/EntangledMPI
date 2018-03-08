#include <stdint.h>
#include <setjmp.h>
#include <pthread.h>
#include <assert.h>
#include <mpi.h>

#ifndef __SHARED_H__
#define __SHARED_H__

#define DEBUG

// Map is a file or API response which maps compute node to a replica node. If the
// hash of the file/API is changed MAP_UPDATED status is used.
enum MapStatus { MAP_NOT_UPDATED, MAP_UPDATED };
typedef uint64_t address; 

#define PTR_ENCRYPT(variable) \
        asm ("xor %%fs:0x30, %0\n\t" \
        "rol $17, %0" \
        : "=r" (variable) \
        : "0" (variable)); 

#define PTR_DECRYPT(variable) \
        asm ("ror $17, %0\n\t" \
        "xor %%fs:0x30, %0" \
        : "=r" (variable) \
        : "0" (variable));

#define Init_Rep(variable) \
        asm ("mov %%rbp, %0\n\t" \
        : "=r" (variable));



enum NodeTransitState { NODE_DATA_NONE, NODE_DATA_SENDER, NODE_DATA_RECEIVER };
enum NodeCheckpointMaster { NO, YES };
enum CkptBackup { BACKUP_NO, BACKUP_YES };
/* 
*  Structure to hold each node's new rank. This will work as a wrapper to 
*  existing comms 
*/
typedef struct Nodes {
	int job_id;
	int rank;
	int age;
	int jobs_count;
	enum NodeTransitState node_transit_state;
	enum NodeCheckpointMaster node_checkpoint_master;
	MPI_Comm world_job_comm; 		// Communicator all all nodes in a job.
	MPI_Comm active_comm;			// Communicator of nodes, one from each job. So these can be called active nodes.
} Node;

typedef struct Jobs {
	int job_id;
	int worker_count;
	int *rank_list;
} Job;

typedef struct ContextContainer {
	address rip;
	address rsp;
	address rbp;
} Context;

#endif
