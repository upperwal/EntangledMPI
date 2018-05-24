#include <stdio.h>
#include "src/replication/rep.h"

extern Node node;

int success[3];

int main(int argc, char** argv){
	MPI_Init(&argc, &argv);

	int rank, size, send, reduce_result;


	MPI_Comm comm = MPI_COMM_WORLD;

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	if(node.rank == 0)
		exit(EXIT_FAILURE);

	send = rank + 1;

	for(int i=0; i<size; i++) {
		MPI_Reduce(&send, &reduce_result, 1, MPI_INT, MPI_MAX, i, comm);

		if(rank == i) {
			if(reduce_result == size)
				success[0] = 1;
		}

		MPI_Reduce(&send, &reduce_result, 1, MPI_INT, MPI_MIN, i, comm);

		if(rank == i) {
			if(reduce_result == 1)
				success[1] = 1;
		}

		MPI_Reduce(&send, &reduce_result, 1, MPI_INT, MPI_SUM, i, comm);

		if(rank == i) {
			if(reduce_result == (size * (size + 1) / 2))
				success[2] = 1;
		}
	}

	if(success[0] == 1 && success[1] == 1 && success[2] == 1) {
		printf("Rank: %d SUCCESS\n", rank);
	}
	else {
		if(success[0] != 1)
			printf("Rank: %d FAIL: MPI_MAX\n", rank);
		if(success[1] != 1)
			printf("Rank: %d FAIL: MPI_MIN\n", rank);
		if(success[2] != 1)
			printf("Rank: %d FAIL: MPI_SUM\n", rank);
	}

	MPI_Finalize();
	return 1;
}
