#include <mpi.h>
#include <stdio.h>
#include <pthread.h>

#include "src/shared.h"
#include "src/replication/rep.h"

address _stackStart; 	// Higher end address (stack grows higher to lower address)

void *rep_thread(void *stackStart) {
	_stackStart = (address *)stackStart;
	sh = 1995;
	printf("Stack Start Address: %p | Value: %p\n", &_stackStart, _stackStart);
}

int MPI_Init(int *argc, char ***argv) {
	if(argc == NULL && argv == NULL) {
		printf("Error: MPI_Init, Message: Pass atleast one argument to MPI_Init function.\n");
		exit(-1);
	}
	
	printf("argc: %p | argv: %p\n", argc, argv);

	pthread_t tid;
	pthread_create(&tid, NULL, rep_thread, argc);
	sleep(1);
	printf("%p\n", tid);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	check_for_map_update();
}
