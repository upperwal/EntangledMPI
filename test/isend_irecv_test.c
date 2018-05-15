#include "include/repmpi.h"

#define SIZE 10

int global_data[SIZE];
int recv_global[SIZE];

int main(int argc, char** argv) {

	int rank, size;
	MPI_Comm comm = MPI_COMM_WORLD;
	int success = 1;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	printf("My Rank: %d\n", rank);

	if(size != 2) {
		printf("Comm size should be exactly 2\n");
		MPI_Abort(comm, 500);
	}

	MPI_Request req;
	MPI_Status stat;

	if(rank  == 0) {
		for(int i=0; i< SIZE; i++) {
			global_data[i] = i;
		}
		printf("SENDING...\n");
		MPI_Isend(global_data, SIZE, MPI_INT, 1, 0, comm, &req);
	} else {
		printf("RECV...\n");
		MPI_Irecv(global_data, SIZE, MPI_INT, 0, 0, comm, &req);
	}

	MPI_Wait(&req, &stat);

	MPI_Irecv(recv_global, SIZE, MPI_INT, 1 - rank, 0, comm, &req);
	MPI_Send(global_data, SIZE, MPI_INT, 1 - rank, 0, comm);

	MPI_Wait(&req, &stat);

	for(int i=0; i< SIZE; i++) {
		if(recv_global[i]!= i) {
			success = 0;
			break;
		}
	}

	if(success == 1) {
		printf("SUCCESS\n");
	}
	else {
		printf("FAIL\n");
	}

	printf("DONE****\n");
}