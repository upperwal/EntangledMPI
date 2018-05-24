#include "include/repmpi.h"

#define SIZE 10

#define SENDER 4
#define RECEIVER 7

int global_data[SIZE];
int recv_global[SIZE];

int recv_buf;

int main(int argc, char** argv) {

	int rank, size;
	MPI_Comm comm = MPI_COMM_WORLD;
	int success[2];
	success[0] = 1;
	success[1] = 1;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	printf("My Rank: %d\n", rank);

	MPI_Request req;
	MPI_Status stat;

	// Test 1 BEGIN
	if(rank  == SENDER) {
		for(int i=0; i< SIZE; i++) {
			global_data[i] = i;
		}
		printf("SENDING...\n");
		MPI_Send(global_data, SIZE, MPI_INT, RECEIVER, 0, comm);
	} else if(rank == RECEIVER) {
		printf("RECV...\n");
		MPI_Irecv(global_data, SIZE, MPI_INT, MPI_ANY_SOURCE, 0, comm, &req);
		MPI_Wait(&req, &stat);

		for(int i=0; i< SIZE; i++) {
			if(global_data[i]!= i) {
				success[0] = 0;
				//printf("NOT MATCHING: %d\n", i);
				break;
			}
		}
		if(SENDER != stat.MPI_SOURCE) {
			printf("Rank: %d | Should receive from %d instead %d\n",rank, SENDER, stat.MPI_SOURCE);
			success[0] = 0;
		}
	}
	// Test 1 ENDS

	MPI_Barrier(MPI_COMM_WORLD);

	// Test 2 BEGIN
	MPI_Irecv(&recv_buf, 1, MPI_INT, (rank - 1 + size) % size, 1, comm, &req);
	MPI_Send(&rank, 1, MPI_INT, (rank + 1) % size, 1, comm);

	MPI_Wait(&req, &stat);

	if(recv_buf != (rank - 1 + size) % size) {
		success[1] = 0;
	}
	// Test 2 ENDS

	if(success[0] == 1 && success[1] == 1) {
		printf("Rank: %d | SUCCESS\n", rank);
	}
	else {
		if(success[0] == 0) {
			printf("Rank: %d | Test 1 FAILS\n", rank);
		}
		if(success[1] == 0) {
			printf("Rank: %d | Test 2 FAILS  | Val: %d | %d\n", rank, recv_buf,  (rank - 1 + size) % size);
		}
		
	}
	MPI_Finalize();
}