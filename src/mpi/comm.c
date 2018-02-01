#include "comm.h"

int init_nodes(char *file_name, Job **job_list, Node *node) {
	parse_map_file(file_name, job_list, node);
}

int parse_map_file(char *file_name, Job **job_list, Node *node) {
	FILE *pointer = fopen(file_name, "r");

	if(pointer == NULL) {
		printf("Cannot open file.\n");
        exit(0);
	}

	int cores, jobs;
	fscanf(pointer, "%d\t%d", &cores, &jobs);
	printf("Cores: %d | Works: %d\n",cores, jobs);

	

	int my_rank;
	PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	*job_list = (Job *)malloc(sizeof(Job) * jobs);
	for(int i=0; i<jobs; i++) {
		int j_id, w_c;

		assert(j_id < jobs && "Check replication map for job id > no of jobs.");
		
		fscanf(pointer, "%d\t%d\t", &j_id, &w_c);
		(*job_list)[j_id].job_id = j_id;
		(*job_list)[j_id].worker_count = w_c;
		(*job_list)[j_id].rank_list = malloc(sizeof(int) * w_c);
		for(int j=0; j<w_c; j++) {
			int w_rank;
			fscanf(pointer, "\t%d", &w_rank);

			if(w_rank == my_rank) {
				(*node).job_id = j_id;
				(*node).rank = my_rank;
				(*node).age = 1;
			}
			
			(*job_list)[j_id].rank_list[j] = w_rank;
		}
	}

	for(int i=0; i<jobs; i++) {
		printf("Original Rank: %d | Rank: %d | Job ID: %d | Worker Count: %d | Worker 1: %d | Worker 2: %d\n", my_rank, (*node).rank, (*job_list)[i].job_id, (*job_list)[i].worker_count, (*job_list)[i].rank_list[0], (*job_list)[i].rank_list[1]);
	}
}
