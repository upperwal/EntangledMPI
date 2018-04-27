#include <stdio.h>
#include <stdlib.h>
#include "src/replication/rep.h"

#define SMALL_BUF_SIZE 5000
#define SLEEP_TIME 2

float *big_buffer;
float *small_buffer;
float *reduce_result;

// Move this to header file
extern int __pass_sender_cont_add;
extern int __pass_receiver_cont_add;

void assign_data(float *buf, int size) {
	for(int i = 0; i < size; i++) {
		buf[i] = i;
	}
}

//void __attribute__((constructor)) calledFirst();



int main(int argc, char **argv) {
	int ***n = argv;
	printf("Add: %p\n", &argc);
	//sleep(20);
	MPI_Init(&argc, &argv);

	int rank, size;

	MPI_Comm comm = MPI_COMM_WORLD;

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	//sleep(10);

	/*if(size < 10) {
		printf("COMM_WORLD size should be >= 10\n");
		MPI_Abort(comm, 1);
	}*/

	rep_malloc((void **)&big_buffer, sizeof(float) * SMALL_BUF_SIZE * size);
	rep_malloc((void **)&small_buffer, sizeof(float) * SMALL_BUF_SIZE);
	rep_malloc((void **)&reduce_result, sizeof(float) * SMALL_BUF_SIZE * size);

	assign_data(big_buffer, SMALL_BUF_SIZE * size);
	assign_data(small_buffer, SMALL_BUF_SIZE);

	// BCAST STARTS
	__pass_sender_cont_add = 1;

	MPI_Bcast(&big_buffer, SMALL_BUF_SIZE * size, MPI_FLOAT, 0 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Bcast(&big_buffer, SMALL_BUF_SIZE * size, MPI_FLOAT, 1 % size, comm);
	sleep(SLEEP_TIME);
	
	MPI_Bcast(&big_buffer, SMALL_BUF_SIZE * size, MPI_FLOAT, 2 % size, comm);
	sleep(SLEEP_TIME);
	
	MPI_Bcast(&big_buffer, SMALL_BUF_SIZE * size, MPI_FLOAT, 3 % size, comm);
	sleep(SLEEP_TIME);
	// BCAST ENDS


	__pass_receiver_cont_add = 1;
	// SCATTER STARTS
	MPI_Scatter(&big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 0 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Scatter(&big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 3 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Scatter(&big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 8 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Scatter(&big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 5 % size, comm);
	sleep(SLEEP_TIME);
	// SCATTER ENDS



	// GATHER STARTS
	MPI_Gather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 8 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Gather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 3 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Gather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 2 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Gather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, 9 % size, comm);
	sleep(SLEEP_TIME);
	// GATHER ENDS



	// ALLGATHER STARTS
	MPI_Allgather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, comm);
	sleep(SLEEP_TIME);

	MPI_Allgather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, comm);
	sleep(SLEEP_TIME);

	MPI_Allgather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, comm);
	sleep(SLEEP_TIME);

	MPI_Allgather(&small_buffer, SMALL_BUF_SIZE, MPI_FLOAT, &big_buffer, SMALL_BUF_SIZE, MPI_FLOAT, comm);
	sleep(SLEEP_TIME);
	// ALLGATHER ENDS


	float r_s = 1, r_r;
	__pass_sender_cont_add = 0;
	__pass_receiver_cont_add = 0;
	// REDUCE STARTS
	MPI_Reduce(&r_s, &r_r, 1, MPI_FLOAT, MPI_MAX, 5 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Reduce(&r_s, &r_r, 1, MPI_FLOAT, MPI_SUM, 2 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Reduce(&r_s, &r_r, 1, MPI_FLOAT, MPI_MIN, 8 % size, comm);
	sleep(SLEEP_TIME);

	MPI_Reduce(&r_s, &r_r, 1, MPI_FLOAT, MPI_SUM, 7 % size, comm);
	sleep(SLEEP_TIME);
	// REDUCE ENDS


	__pass_sender_cont_add = 1;
	__pass_receiver_cont_add = 1;
	// ALL REDUCE STARTS
	MPI_Allreduce(&big_buffer, &reduce_result, SMALL_BUF_SIZE * size, MPI_FLOAT, MPI_MIN, comm);
	sleep(SLEEP_TIME);

	MPI_Allreduce(&big_buffer, &reduce_result, SMALL_BUF_SIZE * size, MPI_FLOAT, MPI_MAX, comm);
	sleep(SLEEP_TIME);

	MPI_Allreduce(&big_buffer, &reduce_result, SMALL_BUF_SIZE * size, MPI_FLOAT, MPI_PROD, comm);
	sleep(SLEEP_TIME);

	MPI_Allreduce(&big_buffer, &reduce_result, SMALL_BUF_SIZE * size, MPI_FLOAT, MPI_MAX, comm);
	sleep(SLEEP_TIME);
	// ALL REDUCE ENDS


	rep_free((void **)&big_buffer);
	rep_free((void **)&small_buffer);
	rep_free((void **)&reduce_result);

	MPI_Finalize();

	printf("*****DONE %d\n", rank);
}
