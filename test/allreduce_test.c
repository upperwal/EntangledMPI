#include <stdio.h>
#include "src/replication/rep.h"

#define DATA_SIZE 4

extern Node node;

int main(int argc, char** argv){
	MPI_Init(&argc, &argv);

	int rank, size, send, reduce_result = -3;

	MPI_Comm comm = MPI_COMM_WORLD;

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	if(node.rank == 0)
		exit(EXIT_FAILURE);

	send = rank;

	MPI_Allreduce(&send, &reduce_result, 1, MPI_INT, MPI_MAX, comm);

	int success = 0;
	if(reduce_result == size - 1)
		success = 1;
	if(success) {
		printf("Rank: %d SUCCESS\n", rank);
	}
	else {
		printf("Rank: %d FAIL\n", rank);
	}
	
	MPI_Finalize();
	return 1;
}
