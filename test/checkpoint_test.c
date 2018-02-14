#include <stdio.h>
#include <mpi.h>
#include "src/replication/rep.h"

int a;

int main(int argc, char** argv) {
	int rank, size, len = 9, color = 0;
	char procName[100];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	printf("Data Seg: %d\n", a);

	MPI_Comm job_comm;

	

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	a = 1995;

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	//char *ckpt_file = "/home/mas/16/cdsabhi/entangledmpi/build_cray/ckpt/rank-%d.ckpt";

	//init_ckpt(ckpt_file);

	PMPI_Finalize();
}
