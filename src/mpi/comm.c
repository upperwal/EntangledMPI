#include "comm.h"

extern Node node;
extern Job *job_list;
extern int *rank_2_job;

extern enum CkptRestore ckpt_restore;

extern MPI_Errhandler ulfm_err_handler;

extern pthread_mutex_t comm_use_mutex;

extern int *rank_ignore_list;

int init_node(char *file_name, Job **job_list, Node *node) {
	
	int my_rank, comm_size;
	PMPI_Comm_rank((*node).rep_mpi_comm_world, &my_rank);
	PMPI_Comm_size((*node).rep_mpi_comm_world, &comm_size);

	// Remember to update this rank after migration.
	(*node).rank = my_rank; 			
	(*node).static_rank = my_rank;		// This won't change

	// Allocating memory for rank ignore list.
	rank_ignore_list = (int *)calloc(comm_size, sizeof(int));

	debug_log_i("Initiating Node and Jobs data from file.");

	// If job_id = -1 not set, as "node" is a global variable and is cleared by default
	// job id 0 in the map file will not be initiated properly.
	(*node).job_id = -1;

	parse_map_file(file_name, job_list, node, &ckpt_restore);
	update_comms();

	// parse_map_file will set "node_transit_state" to "NODE_DATA_RECEIVER"
	// because it thinks all nodes are newly added to this job.
	// So a correction has to be made.
	(*node).node_transit_state = NODE_DATA_NONE;

	/*for(int i=0; i<(*node).jobs_count; i++) {
		debug_log_i("[Node Init] My Job ID: %d | Node Transit: %d | Job ID: %d | Worker Count: %d | Worker 1: %d | Worker 2: %d", (*node).job_id, (*node).node_transit_state, (*job_list)[i].job_id, (*job_list)[i].worker_count, (*job_list)[i].rank_list[0], (*job_list)[i].rank_list[1]);
	}*/
}

int parse_map_file(char *file_name, Job **job_list, Node *node, enum CkptRestore *ckpt_restore) {
	
	if(*ckpt_restore == RESTORE_YES) {
		return 0;
	}

	FILE *pointer = fopen(file_name, "r");

	if(pointer == NULL) {
		log_e("Cannot open replication map file.");
        exit(0);
	}

	int cores, jobs;
	fscanf(pointer, "%d\t%d", &cores, &jobs);

	// Reset node_checkpoint_master
	(*node).node_checkpoint_master = NO;
	(*node).node_transit_state = NODE_DATA_NONE;

	// TODO: Optimise re-allocation of memory.
	for(int i=0; i<(*node).jobs_count; i++) {
		free((*job_list)[i].rank_list);
	}
	free(*job_list);

	int my_rank = (*node).rank;

	if(rank_2_job == NULL) {
		rank_2_job = malloc(sizeof(int) * cores);
	}

	*job_list = (Job *)malloc(sizeof(Job) * jobs);
	for(int i=0; i<jobs; i++) {
		int j_id, w_c, update_bit;
		int w_rank;
		
		fscanf(pointer, "%d\t%d\t%d\t", &update_bit, &j_id, &w_c);

		assert(j_id < jobs && "Check replication map for job id > no of jobs.");
		assert(w_c > 0 && "Worker count for each job should be greater than zero.");

		/*if(update_bit == 0) {
			if((*node).job_id == j_id) {
				(*node).node_transit_state = NODE_DATA_NONE;
			}
			//continue;
		}*/

		(*job_list)[j_id].job_id = j_id;
		(*job_list)[j_id].worker_count = w_c;
		(*job_list)[j_id].rank_list = malloc(sizeof(int) * w_c);

		

		for(int j=0; j<w_c; j++) {
			
			fscanf(pointer, "\t%d", &w_rank);

			rank_2_job[w_rank] = j_id;

			if(j == 0 && w_rank == my_rank) {
				(*node).node_checkpoint_master = YES;
				debug_log_i("CK MASTER: %d", (*node).node_checkpoint_master);
			}

			if(w_rank == my_rank && update_bit == 1) {
				if((*node).job_id != j_id) {
					(*node).age = 1;
					(*node).job_id = j_id;
					(*node).node_transit_state = NODE_DATA_RECEIVER;
				}
				else {
					(*node).age++;
					// 1. Job Id same as previous but some new rank added to this job.
					// 2. First job in the map needs to send data to that rank.
					// 3. Sends data only if workers > 1. If workers == 1, then only this
					//    rank exists.
					if(j == 0 && w_c > 1) {
						(*node).node_transit_state = NODE_DATA_SENDER;	
					}
					else {
						(*node).node_transit_state = NODE_DATA_NONE;
					}
				}
				(*node).jobs_count = jobs;
				
			}
			
			(*job_list)[j_id].rank_list[j] = w_rank;
		}
	}

	for(int i=0; i<jobs; i++) {
		debug_log_i("[Rep File Update] MyJobId: %d | Job ID: %d | Worker Count: %d | Worker 1: %d | Worker 2: %d | Checkpoint: %d", (*node).job_id, (*job_list)[i].job_id, (*job_list)[i].worker_count, (*job_list)[i].rank_list[0], (*job_list)[i].rank_list[1], (*node).node_checkpoint_master);
	}

	debug_log_i("node_transit_state: Sender: %d | Recv: %d | None: %d", (*node).node_transit_state == NODE_DATA_SENDER, (*node).node_transit_state == NODE_DATA_RECEIVER, (*node).node_transit_state == NODE_DATA_NONE);
}

// responsible to update 'world_job_comm' and 'active_comm' [defined in src/shared.h]
// 'world_job_comm': Communicator all all nodes in a job.
// 'active_comm': Communicator of nodes, one from each job. So these can be called active nodes.
void update_comms() {
	int color = 0, rank_key = node.job_id;

	// Explain
	/*debug_log_i("Before Lock...");
	pthread_mutex_lock(&comm_use_mutex);
	debug_log_i("After Lock...");*/

	// Although misguiding 'node.node_checkpoint_master' is not just used to mark a node
	// which takes checkpoint on behalf of a job but it is also used to do communications
	// amoung other jobs. Then the result is send to all nodes of 'this' job.
	
	// 'node.node_checkpoint_master == NO' are the nodes not responsible for checkpointing
	// in this job.
	if(node.node_checkpoint_master == NO) {
		color = MPI_UNDEFINED;
	}

	debug_log_i("UPDATE_COMM:- node.rep_mpi_comm_world add: %p | node.active_comm add: %p | node add: %p", &(node.rep_mpi_comm_world), &(node.active_comm), &node);
	PMPI_Comm_split(node.rep_mpi_comm_world, color, rank_key, &(node.active_comm));
	//PMPI_Comm_set_errhandler(node.active_comm, ulfm_err_handler);

	// Test
	int rank;

	if(node.active_comm != MPI_COMM_NULL) {
		PMPI_Comm_rank(node.active_comm, &rank);
		debug_log_i("Job ID: %d | active_comm Rank: %d", node.job_id, rank);
	}
	

	color = node.job_id;
	if(node.node_checkpoint_master == YES) {
		rank_key = 0;
	}
	else {
		rank_key = 1;
	}

	PMPI_Comm_split(node.rep_mpi_comm_world, color, rank_key, &(node.world_job_comm));
	//PMPI_Comm_set_errhandler(node.world_job_comm, ulfm_err_handler);

	//pthread_mutex_unlock(&comm_use_mutex);

	// Test
	PMPI_Comm_rank(node.world_job_comm, &rank);
	debug_log_i("Job ID: %d | world_job_comm Rank: %d", node.job_id, rank);
	debug_log_i("Checkpoint MASTER: %d", node.node_checkpoint_master == YES);
}

void acquire_comm_lock() {
	pthread_mutex_lock(&comm_use_mutex);
}

void release_comm_lock() {
	pthread_mutex_unlock(&comm_use_mutex);
}

/* Returns 1 if comm is valid on this node, else 0. */
int create_migration_comm(MPI_Comm *job_comm, int *rep_flag, enum CkptRestore *ckpt_restore) {
	/* 								this^
	*  This comm will contain all the processes which are either sending or receiving 
	*  replication data. 
	*/
	int color, key, flag;

	if(*ckpt_restore == RESTORE_YES) {
		*rep_flag = 0;
		return 1;
	}

	// TODO: Comm was getting corrupted (review this)
	//pthread_mutex_lock(&comm_use_mutex);

	if(node.node_transit_state != NODE_DATA_NONE) {
		color = node.job_id;
		if(node.node_transit_state == NODE_DATA_SENDER) {
			key = 0;
		}
		else {
			key = 1;
		}
		flag = 1;
		debug_log_i("Comm Created. | Job: %d", node.job_id);
	}
	else {
		color = MPI_UNDEFINED;
		flag = 0;
		debug_log_i("No Comm Created. | Job: %d", node.job_id);
	}

	*rep_flag = flag;

	debug_log_i("Color: %d | key: %d | job_comm: %p", color, key, job_comm);

	PMPI_Comm_split(node.rep_mpi_comm_world, color, key, job_comm);

	//pthread_mutex_unlock(&comm_use_mutex);

	debug_log_i("Create Migration Comm: flag: %d | ckpt master: %d", flag, node.node_checkpoint_master);
	
	return (flag || node.node_checkpoint_master);
}

// TODO: Print only workers which are assigned.
void print_job_list() {
	for(int i=0; i<node.jobs_count; i++) {
		
		debug_log_i("MyJobId: %d | Job ID: %d | Worker Count: %d | Worker 1: %d | Worker 2: %d | Checkpoint: %d", node.job_id, job_list[i].job_id, job_list[i].worker_count, job_list[i].rank_list[0], job_list[i].rank_list[1], node.node_checkpoint_master);

	}
}
