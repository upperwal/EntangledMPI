#include <stdio.h>
#include "src/replication/heapseg.h"

int main(int argc, char** argv) {
	int rank, size, len, color = 0;
	char procName[100];
	MPI_Comm comm;

	comm = MPI_COMM_WORLD;

	PMPI_Init(NULL, NULL);

	PMPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int *a, *b;
	char *r;
	if(rank == 0) {
		rep_malloc(&a, sizeof(int));
		*a = 20;
		assign_malloc_context(&a, &b);		// For "b = a" case, no new memory is allocated.
		rep_malloc(&r, sizeof(char));
		*r = 'p';
	}

	transfer_heap_seg(comm);

	if(rank == 1) {
		if(*a == 20 && *r == 'p' && a == b) {
			printf("SUCCESS\n");
		}
		else {
			printf("FAILED\n");
		}
	}
	
	#ifdef DEBUG
	printf("Rank: %d | Value of a: %d | Adddress of a: %p\n", rank, *a, a);
	printf("Rank: %d | Value of a: %d | Adddress of a: %p\n", rank, *b, b);
	#endif

	PMPI_Finalize();
}
