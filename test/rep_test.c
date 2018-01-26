#include <stdio.h>
#include "src/replication/rep.h"
#include "src/replication/dataseg.h"

#include <pthread.h>

int a;

void *threadFunction(void *var) {
	
	int a = 9;
	printf("Thread: Add: %p\n", &var);
}

int main(int argc, char** argv){
	int rank, size, len;
	char procName[100];

	pthread_t tid;

	pthread_create(&tid, NULL, threadFunction, NULL);


	//stackMig(2);

	readProcMapFile();

	//printf("Init: %p | Uninit: %p\n", &a, &b);

	/*
	MPI_Comm comm = MPI_COMM_WORLD;

	MPI_Init(&argc, &argv);

	MPI_Replicate();

	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	printf("Hello after size and rank functions\n");

	MPI_Barrier(comm);

	//int *temp = (int *) malloc(sizeof(int)*20);

	MPI_Get_processor_name(procName,&len);

	printf("Hello from Process %d of %d on %s\n", rank, size, procName);


	MPI_Finalize();*/
}
