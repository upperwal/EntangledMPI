#include <stdio.h>
#include "src/replication/rep.h"

#include <pthread.h>

int a = 80;
int b;

void *threadFunction(void *var) {
	
	int a = 9;
	printf("Thread: Add: %p\n", &var);
}

int main(int argc, char** argv){
	int rank, size, len, oo = 9;
	char procName[100];

	pthread_t tid;

	int* ab;
	Init_Rep(ab);

	pthread_create(&tid, NULL, threadFunction, NULL);

	MPI_Init(ab, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//stackMig(2);
	if(rank == 0 || rank == 2) {
		oo = 65;
	}

	

	
	//readProcMapFile();
	//printf("In Main | main thread\n");

	sleep(10);
	MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);


	printf("Rank: %d | [Users program] Value: %d | address: %p\n", rank, oo, &oo);
	//printf("Init: %p | Uninit: %p\n", &a, &b);


	MPI_Finalize();

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
