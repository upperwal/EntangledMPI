#include <mpi.h>
#include <stdio.h>

#include "src/shared.h"
#include "src/replication/rep.h"

void *rep_thread_init(void *stackStart) {
	address _stackStart; 	// Higher end address (stack grows higher to lower address)
	_stackStart = (address *)stackStart;
	//sh = 1995;
	printf("Stack Start Address: %p | Value: %p\n", &_stackStart, _stackStart);

	// Start checking for any map file updates.
	// check_map_file_changes();
	pthread_mutex_lock(&rep_time_mutex);
	map_status = MAP_UPDATED;

	pthread_mutex_lock(&global_mutex);

	// Replica creation code
	printf("Rep Code\n");

	map_status = MAP_NOT_UPDATED;

	pthread_mutex_unlock(&global_mutex);
	pthread_mutex_unlock(&rep_time_mutex);
}

int MPI_Init(int *argc, char ***argv) {
	// Lock global mutex. This mutex will always be locked when user program is executing.
	pthread_mutex_init(&global_mutex, NULL);
	pthread_mutex_init(&rep_time_mutex, NULL);
	
	pthread_mutex_lock(&global_mutex);
	
	if(argc == NULL && argv == NULL) {
		printf("Error: MPI_Init, Message: Pass &argcto MPI_Init function.\n");
		exit(-1);
	}
	
	printf("argc: %p | argv: %p\n", argc, argv);

	pthread_t tid;
	pthread_create(&tid, NULL, rep_thread_init, argc);
	sleep(1);
	printf("%p\n", tid);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	is_file_update_set();
}
