#include <stdio.h>
#include "src/replication/dataseg.h"

int a;

int main(int argc, char** argv) {
	int rank, size, len, color = 0;
	char procName[100];

	PMPI_Init(NULL, NULL);

	MPI_Comm job_comm;

	PMPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0 || rank == 1)
		color = 1;
	else 
		color = MPI_UNDEFINED;

	PMPI_Comm_split(MPI_COMM_WORLD, color, rank, &job_comm);

	
	//readProcMapFile();
	if(rank == 0)
		a = 10;

	if(rank == 0 || rank == 1) {
		transfer_data_seg(job_comm);
	}

	if(rank == 1) {
		if(a == 10) {
			printf("SUCCESS\n");
		} 
		else {
			printf("FAILED\n");
		}
	}

	PMPI_Finalize();
}
