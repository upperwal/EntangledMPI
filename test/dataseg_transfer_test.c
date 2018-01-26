#include <stdio.h>
#include "src/replication/dataseg.h"

int a;

int main(int argc, char** argv){
	int rank, size, len;
	char procName[100];

	MPI_Init(NULL, NULL);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0)
		a = 10;

	transferDataSeg(rank, MPI_COMM_WORLD);

	if(rank == 1) {
		if(a == 10) {
			printf("SUCCESS\n");
		} 
		else {
			printf("FAILED\n");
		}
	}

	MPI_Finalize();
}
