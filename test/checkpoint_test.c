#include <stdio.h>
#include <mpi.h>
#include "src/replication/rep.h"

int a;

int main(int argc, char** argv) {
	int rank, size, len = 9, color = 0;
	int *ptr;
	char procName[100];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	rep_malloc(&ptr, sizeof(int));
	printf("Data Seg: %d | Malloc: %d\n", a, *ptr);

	MPI_Comm job_comm;

	

	//sleep(10);
	if(rank == 0) {
		a = 1995;
		MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
	}
	else {
		MPI_Recv(&a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Rank: 1 | Value: %d\n", a);
	}

	
	
	*ptr = 700;

	/*sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);*/

	printf("[2] Data Seg: %d | Malloc: %d\n", a, *ptr);

	//char *ckpt_file = "/home/mas/16/cdsabhi/entangledmpi/build_cray/ckpt/rank-%d.ckpt";

	//init_ckpt(ckpt_file);

	PMPI_Finalize();
}
