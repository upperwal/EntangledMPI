#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "src/replication/rep.h"

#include <pthread.h>

int initSeg = 80;

extern Node node;

void f4(int *a) {

	*a = 96;

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

	sleep(10);
	MPI_Send(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
}

void f3(int *a) {
	f4(a);
}

void f2(int *a) {
	f3(a);
} 

void f1(int *a) {
	f2(a);
}

int main(int argc, char** argv){
	int rank, size = 9, len, oo = 9;
	char procName[100];
	int *a, *b;
	char *c;

	MPI_Init(&argc, &argv);

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

	pid_t pid;

	pid = getpid();

	printf("Process ID: %d\n", pid);

	/*if(node.rank == 1)
		kill(pid, SIGBUS);*/

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	printf("[USER CODE] Rank Address: %p\n", &rank);

	/*if(rank == 1)
		readProcMapFile();*/

	if(rank == 0) {
		rep_malloc(&a, sizeof(int));
		*a = 43;
		initSeg = 1945;
		f1(&oo);
		
	}
	else {
		rep_malloc(&b, sizeof(int));
		*b = 34;
		oo = 77;
		initSeg = 200000;

		sleep(10);
		MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		printf("[User Code] BEFORE 1 | Rank: %d\n", rank);

		sleep(10);
		MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		printf("[User Code] BEFORE 2 | Rank: %d\n", rank);

		sleep(10);
		MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		printf("[User Code] BEFORE 3 | Rank: %d\n", rank);

		sleep(10);
		MPI_Send(&rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		printf("[User Code] BEFORE 4 | Rank: %d\n", rank);
	}

	printf("[User Code] Rank: %d\n", rank);

	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0)
		printf("[Heap Seg] Rank: %d | [Users program] Value: %d | address: %p\n", rank, *a, a);

	printf("[Data Seg] Rank: %d | [Users program] Value: %d | address: %p\n", rank, initSeg, &initSeg);
	printf("[Stack Seg] Rank: %d | [Users program] Value: %d | address: %p\n", rank, oo, &oo);
	
	if(rank == 0) {
		if(*a == 43)
			printf("[Heap Seg] Rank: %d | SUCCESS\n", rank);
		else
			printf("[Heap Seg] Rank: %d | FAIL\n", rank);

		if(initSeg == 1945) 
			printf("[Data Seg] Rank: %d | SUCCESS\n", rank);
		else
			printf("[Data Seg] Rank: %d | FAIL\n", rank);
		
		if(oo == 96)
			printf("[Stack Seg] Rank: %d | SUCCESS\n", rank);
		else
			printf("[Stack Seg] Rank: %d | FAIL\n", rank);
	}
	else if(rank == 1) {
		if(*b == 34)
			printf("[Heap Seg] Rank: %d | SUCCESS\n", rank);
		else
			printf("[Heap Seg] Rank: %d | FAIL\n", rank);

		if(initSeg == 200000) 
			printf("[Data Seg] Rank: %d | SUCCESS\n", rank);
		else
			printf("[Data Seg] Rank: %d | FAIL\n", rank);
		
		if(oo == 77)
			printf("[Stack Seg] Rank: %d | SUCCESS\n", rank);
		else
			printf("[Stack Seg] Rank: %d | FAIL\n", rank);
	}

	MPI_Finalize();
}
