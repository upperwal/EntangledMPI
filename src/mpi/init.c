#include <mpi.h>
#include <stdio.h>

#include "src/shared.h"
#include "src/replication/rep.h"
#include "src/misc/file.h"
#include "src/mpi/comm.h"

#define REP_THREAD_SLEEP_TIME 3

jmp_buf context;
address stackHigherAddress, stackLowerAddress;
enum MapStatus map_status;

/* pthread mutex */
pthread_mutex_t global_mutex;
pthread_mutex_t rep_time_mutex;

Job *job_list;
Node node;

void *rep_thread_init(void *_stackHigherAddress) {
	 	// Higher end address (stack grows higher to lower address)
	stackHigherAddress = (address *)_stackHigherAddress;
	char *map_file = "/home/mas/16/cdsabhi/entangledmpi/build_cray/replication.map";
	time_t last_update;

	// this comm contains sender and receiver nodes during replication.
	MPI_Comm job_comm;
	
	// First run does not create replicas, only correct ranks are initialised.
	set_last_update(map_file, &last_update);
	
	// job_list and node declared in shared.h
	init_node(map_file, &job_list, &node);
	//printf("Stack Start Address: %p | Value: %p\n", &stackHigherAddress, stackHigherAddress);

	// Start checking for any map file updates.
	// check_map_file_changes();
	while(1) {
		if(is_file_modified(map_file, &last_update)) {
			
			printf("Inside Modified file\n");

			parse_map_file(map_file, &job_list, &node);
			
			if( create_migration_comm(&job_comm) ) {
				
				pthread_mutex_lock(&rep_time_mutex);
				
				map_status = MAP_UPDATED;
				printf("Modified Signal ON\n");
				
				pthread_mutex_lock(&global_mutex);
				printf("Main thread blocked\n");

				// Replica creation code
				initRep(job_comm);

				map_status = MAP_NOT_UPDATED;

				pthread_mutex_unlock(&global_mutex);
				pthread_mutex_unlock(&rep_time_mutex);
			}
			
		}
		sleep(REP_THREAD_SLEEP_TIME);
		printf("Checking...\n");
	}
	
	
}

int MPI_Init(int *argc, char ***argv) {
	PMPI_Init(NULL, argv);

	printf("Address Stack new: %p\n", argc);
	// Lock global mutex. This mutex will always be locked when user program is executing.
	pthread_mutex_init(&global_mutex, NULL);
	pthread_mutex_init(&rep_time_mutex, NULL);
	
	pthread_mutex_lock(&global_mutex);

	
	
	if(argc == NULL && argv == NULL) {
		printf("Error: MPI_Init, Message: Pass &argc to MPI_Init function.\n");
		exit(-1);
	}
	
	printf("argc: %p | argv: %p\n", argc, argv);

	pthread_t tid;
	pthread_create(&tid, NULL, rep_thread_init, argc);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	printf("In MPI_Send()\n");
	is_file_update_set();
}
