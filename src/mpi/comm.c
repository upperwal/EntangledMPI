#include "comm.h"

extern Node node;

extern enum CkptBackup ckpt_backup;

int init_node(char *file_name, Job **job_list, Node *node) {
	printf("Initiating Node and Jobs data from file.\n");
	
	int my_rank;
	PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	// Remember to update this rank after migration.
	(*node).rank = my_rank;

	// If job_id = -1 not set, as "node" is a global variable and is cleared by default
	// job id 0 in the map file will not be initiated properly.
	(*node).job_id = -1;

	parse_map_file(file_name, job_list, node, &ckpt_backup);

	// parse_map_file will set "node_transit_state" to "NODE_DATA_RECEIVER"
	// because it thinks all nodes are newly added to this job.
	// So a correction has to be made.
	(*node).node_transit_state = NODE_DATA_NONE;

	for(int i=0; i<(*node).jobs_count; i++) {
		printf("[Node Init] Original Rank: %d | Rank: %d | Node Job ID: %d | Node Transit: %d | Job ID: %d | Worker Count: %d | Worker 1: %d | Worker 2: %d\n", my_rank, (*node).rank, (*node).job_id, (*node).node_transit_state, (*job_list)[i].job_id, (*job_list)[i].worker_count, (*job_list)[i].rank_list[0], (*job_list)[i].rank_list[1]);
	}
}

int parse_map_file(char *file_name, Job **job_list, Node *node, enum CkptBackup *ckpt_backup) {
	
	if(*ckpt_backup == BACKUP_YES) {
		return 0;
	}

	FILE *pointer = fopen(file_name, "r");

	if(pointer == NULL) {
		printf("Cannot open file.\n");
        exit(0);
	}

	int cores, jobs;
	fscanf(pointer, "%d\t%d", &cores, &jobs);

	// TODO: Optimise re-allocation of memory.
	for(int i=0; i<(*node).jobs_count; i++) {
		free((*job_list)[i].rank_list);
	}
	free(*job_list);

	int my_rank = (*node).rank;
	

	*job_list = (Job *)malloc(sizeof(Job) * jobs);
	for(int i=0; i<jobs; i++) {
		int j_id, w_c, update_bit;
		int w_rank;
		
		fscanf(pointer, "%d\t%d\t%d\t", &update_bit, &j_id, &w_c);

		assert(j_id < jobs && "Check replication map for job id > no of jobs.");
		assert(w_c > 0 && "Worker count for each job should be greater than zero.");

		if(update_bit == 0) {
			if((*node).job_id == j_id) {
				(*node).node_transit_state = NODE_DATA_NONE;
			}
			//continue;
		}

		(*job_list)[j_id].job_id = j_id;
		(*job_list)[j_id].worker_count = w_c;
		(*job_list)[j_id].rank_list = malloc(sizeof(int) * w_c);

		

		for(int j=0; j<w_c; j++) {
			
			fscanf(pointer, "\t%d", &w_rank);

			if(j == 0 && w_rank == my_rank) {
				(*node).node_checkpoint_master = YES;
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
		printf("[Rep File Update] Original Rank: %d | Rank: %d | Job ID: %d | Worker Count: %d | Worker 1: %d | Worker 2: %d | Checkpoint: %d\n", my_rank, (*node).rank, (*job_list)[i].job_id, (*job_list)[i].worker_count, (*job_list)[i].rank_list[0], (*job_list)[i].rank_list[1], (*node).node_checkpoint_master);
	}
}

/* Returns 1 if comm is valid on this node, else 0. */
int create_migration_comm(MPI_Comm *job_comm, int *rep_flag, enum CkptBackup *ckpt_backup) {
	/* 								this^
	*  This comm will contain all the processes which are either sending or receiving 
	*  replication data. 
	*/
	int color, key, flag;

	if(*ckpt_backup == BACKUP_YES) {
		return 1;
		*rep_flag = 0;
	}

	if(node.node_transit_state != NODE_DATA_NONE) {
		color = node.job_id;
		if(node.node_transit_state == NODE_DATA_SENDER) {
			key = 0;
		}
		else {
			key = 1;
		}
		flag = 1;
		printf("Original Rank: %d | comm created. | Job: %d\n", node.rank, node.job_id);
	}
	else {
		color = MPI_UNDEFINED;
		flag = 0;
		printf("Original Rank: %d | No comm created. | Job: %d\n", node.rank, node.job_id);
	}

	*rep_flag = flag;

	PMPI_Comm_split(MPI_COMM_WORLD, color, key, job_comm);

	printf("Create Migration Comm: Rank: %d | FLAG_OR: %d | flag: %d | ckpt: %d\n", node.rank, (flag || node.node_checkpoint_master), flag, node.node_checkpoint_master);
	return (flag || node.node_checkpoint_master);
}
