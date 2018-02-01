#include <mpi.h>
#include <stdio.h>

#include "src/shared.h"
#include "src/replication/rep.h"
#include "src/misc/file.h"

#define REP_THREAD_SLEEP_TIME 3

void *rep_thread_init(void *stackStart) {
	address _stackStart; 	// Higher end address (stack grows higher to lower address)
	_stackStart = (address *)stackStart;
	char *map_file = "/home/mas/16/cdsabhi/entangledmpi/build_cray/replication.map";
	time_t last_update;
	
	// First run does not create replicas, only correct ranks are initialised.
	set_last_update(map_file, &last_update);
	
	init_nodes(map_file, &job_list, &node);
	printf("Stack Start Address: %p | Value: %p\n", &_stackStart, _stackStart);

	// Start checking for any map file updates.
	// check_map_file_changes();
	while(1) {
		if(is_file_modified(map_file, &last_update)) {
			printf("Inside Modified file\n");
			pthread_mutex_lock(&rep_time_mutex);
			map_status = MAP_UPDATED;
			printf("Modified Signal ON\n");

			pthread_mutex_lock(&global_mutex);
			printf("Main thread blocked\n");

			// Replica creation code
			initRep();

			map_status = MAP_NOT_UPDATED;

			pthread_mutex_unlock(&global_mutex);
			pthread_mutex_unlock(&rep_time_mutex);
		}
		sleep(REP_THREAD_SLEEP_TIME);
		printf("Checking...\n");
	}
	
	
}

int MPI_Init(int *argc, char ***argv) {
	PMPI_Init(argc, argv);

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

	printf("%p\n", tid);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	printf("In MPI_Send()\n");
	is_file_update_set();
}
