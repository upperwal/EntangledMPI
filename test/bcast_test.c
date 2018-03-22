#include <stdio.h>
#include "src/replication/rep.h"

#define DATA_SIZE 4

extern Node node;

int main(int argc, char** argv){
	MPI_Init(&argc, &argv);

	int rank, size, *arr_buf;

	MPI_Comm comm = MPI_COMM_WORLD;

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	if(node.rank == 0)
		exit(EXIT_FAILURE);

	rep_malloc(&arr_buf, sizeof(int) * DATA_SIZE);

	if(rank == 0) {
		for(int i=0; i<DATA_SIZE; i++) {
			arr_buf[i] = i;
		}
	}
	
	MPI_Bcast(arr_buf, DATA_SIZE, MPI_INT, 0, comm);

	int success = 1;
	for(int i=0; i<DATA_SIZE; i++) {
		if(arr_buf[i] != i)
			success = 0;
	}

	if(success) {
		printf("Rank: %d SUCCESS\n", rank);
	}
	else {
		printf("Rank: %d FAIL\n", rank);
	}
	
	MPI_Finalize();
	return 1;
}
