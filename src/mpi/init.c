#include <mpi.h>

#ifdef OPEN_MPI
#include <mpi-ext.h>
#endif

#include <stdio.h>
#include <signal.h>

#include "src/shared.h"
#include "src/replication/rep.h"
#include "src/misc/file.h"
#include "src/misc/network.h"
#include "src/mpi/comm.h"
#include "src/checkpoint/full_context.h"
#include "src/mpi/ulfm.h"
#include "src/mpi/async.h"

#define REP_THREAD_SLEEP_TIME 3
#define NO_TRIALS 10
#define DEFINE_BUFFER(_buf, buf) void **_buf = buf;
#define SET_RIGHT_S_BUFFER(_buffer) __pass_sender_cont_add ? *_buffer : _buffer
#define SET_RIGHT_R_BUFFER(_buffer) __pass_receiver_cont_add ? *_buffer : _buffer

jmp_buf context;
address stackHigherAddress, stackLowerAddress;
enum MapStatus map_status;

address stackStart;

/* pthread mutex */
pthread_mutex_t global_mutex;
pthread_mutex_t rep_time_mutex;
pthread_mutex_t comm_use_mutex;

pthread_mutexattr_t attr_comm_to_use;

Job *job_list;
Node node;
int *rank_2_job = NULL;

char *map_file = "./replication.map";
#define ckpt_file "./ckpt/rank-%d.ckpt"
char *network_stat_file = "./network.stat";

// Restore from checkpoint files: YES | Do not restore: NO
enum CkptBackup ckpt_backup;

// this comm contains sender and receiver nodes during replication.
MPI_Comm job_comm;

MPI_Errhandler ulfm_err_handler;

// Non zero when container address is passed to MPI_* functions.
// When pointers are passed to functions by value (addresses returned by malloc). 
// These pointer values are pushed to the stack during function execution.
// During stack replication or checkpointing, these address are send
// to the corresponding node where these addresses are invalid.
int __pass_sender_cont_add;
int __pass_receiver_cont_add;

// This variable is used in non-collective calls to ignore process failure errors.
// Error handler uses collective calls and not call processes/ranks will be calling 
// *send and *recv. Hance it could result in a deadlock, so better ignore and
// correct comm in a collective call (happening sometime after this call)
int __ignore_process_failure;

extern Malloc_list *head;

void __attribute__((constructor)) calledFirst(void)
{	
	int a;
    
    stackStart = &a;
    /*int hang = 1;
    debug_log_i("Program Hanged");
	while(hang) {
		sleep(2);
	}*/
}

void *rep_thread_init(void *_stackHigherAddress) {
	 	// Higher end address (stack grows higher to lower address)
	stackHigherAddress = (address *)_stackHigherAddress;
	
	time_t last_update;
	int rep_flag;
	
	// First run does not create replicas, only correct ranks are initialised.
	set_last_update(map_file, &last_update);
	
	//printf("Stack Start Address: %p | Value: %p\n", &stackHigherAddress, stackHigherAddress);

	while(1) {

		// Start checking for any map file updates.
		if(is_file_modified(map_file, &last_update, &ckpt_backup)) {
			
			debug_log_i("Inside Modified file");

			parse_map_file(map_file, &job_list, &node, &ckpt_backup);

			update_comms();
			
			if(create_migration_comm(&job_comm, &rep_flag, &ckpt_backup) ) {
				
				pthread_mutex_lock(&rep_time_mutex);
				
				map_status = MAP_UPDATED;
				debug_log_i("Modified Signal ON");
				
				pthread_mutex_lock(&global_mutex);
				log_i("Main thread blocked");

				if(ckpt_backup == BACKUP_YES) {
					// Checkpoint Backup
					log_i("Checkpoint restore started...");

					// ckpt restore code
					init_ckpt_restore(ckpt_file);

					ckpt_backup = BACKUP_NO;	// Very imp, do not miss this.
				}
				else {
				
					// Checkpoint creation
					if(node.node_checkpoint_master == YES)
						init_ckpt(ckpt_file);

					// Replica creation
					if(rep_flag)
						init_rep(job_comm);
				}

				map_status = MAP_NOT_UPDATED;

				pthread_mutex_unlock(&global_mutex);
				pthread_mutex_unlock(&rep_time_mutex);
			}
			
		}
		sleep(REP_THREAD_SLEEP_TIME);
		debug_log_i("File Check Sleep");
	}
}

int MPI_Init(int *argc, char ***argv) {

	int ckpt_bit;

	/*if(mcheck(dyn_mem_err_hook) != 0) {
		log_e("mcheck failed.");
	}
	else {
		log_i("############# MCHECK USED.");
	}*/

	PMPI_Init(argc, argv);

	PMPI_Comm_create_errhandler(rep_errhandler, &ulfm_err_handler);
	//PMPI_Comm_set_errhandler(MPI_COMM_WORLD, ulfm_err_handler);

	PMPI_Comm_dup(MPI_COMM_WORLD, &(node.rep_mpi_comm_world));
	PMPI_Comm_set_errhandler(node.rep_mpi_comm_world, ulfm_err_handler);
	//PMPI_Comm_set_errhandler(MPI_COMM_WORLD, ulfm_err_handler);

	pthread_mutexattr_settype(&attr_comm_to_use, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&global_mutex, NULL);
	pthread_mutex_init(&rep_time_mutex, NULL);
	pthread_mutex_init(&comm_use_mutex, &attr_comm_to_use);

	// Lock global mutex. This mutex will always be locked when user program is executing.
	pthread_mutex_lock(&global_mutex);

	// job_list and node declared in shared.h
	init_node(map_file, &job_list, &node);

	debug_log_i("Initialising MPI.");
	debug_log_i("Node variable address: %p", &node);
	
	/*if(argv == NULL) {
		debug_log_e("You must pass &argv to MPI_Init()");
		exit(0);
	}*/

	debug_log_i("WORLD_COMM Dup: %p", node.rep_mpi_comm_world);

	address temp_stackStart;

	// Getting RBP only works if optimisation level is zero (O0).
	// O1 removes RBP's use.
	/*Init_Rep(stackStart);	// a macro to fetch RBP of main function.
	if(stackStart == 0) {
		// OS kept values of program arguments this this address which is below main function frame.
		// Writable to the user and same for each program. No harm to overwrite this data as it is same for
		// all the nodes. [Same arguments passed]
		stackStart = **argv;
	}*/

	//stackStart = *argv;

	/*PMPI_Allreduce(&stackStart, &temp_stackStart, sizeof(address), MPI_BYTE, MPI_BOR, node.rep_mpi_comm_world);

	if(stackStart != temp_stackStart) {
		PMPI_Abort(node.rep_mpi_comm_world, 100);
		exit(2);
	}
*/
	debug_log_i("Address Stack new: %p | argv add: %p", stackStart, argv);

	ckpt_bit = does_ckpt_file_exists(ckpt_file);

	if(ckpt_bit) {
		ckpt_backup = BACKUP_YES;
	}

	// Save hostname of this host for process manager and fault injector.
	network_stat_init(network_stat_file);

	pthread_t tid;
	pthread_create(&tid, NULL, rep_thread_init, stackStart);

	if(ckpt_bit) {
		while(is_file_update_set() != 1);
	}

	PMPI_Barrier(node.rep_mpi_comm_world);
}

int MPI_Finalize(void) {
	return MPI_Barrier(node.rep_mpi_comm_world);//PMPI_Finalize();
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
	*rank = node.job_id;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
	*size = node.jobs_count;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	debug_log_i("In MPI_Send()");
	is_file_update_set();
	acquire_comm_lock();
	debug_log_i("In MPI_Send() after is_file_update_set");

	__ignore_process_failure = 1;

	MPI_Comm *comm_to_use;

	if(comm == MPI_COMM_WORLD) {
		// So that comm err handler do not identify it as rep_mpi_comm_world
		// It will ignore it and will not enter the if statement.
		// Note: If it enters, process will hang as functions inside are
		// collective.
		//PMPI_Comm_dup(node.rep_mpi_comm_world, &comm_to_use);

		comm_to_use = &(node.rep_mpi_comm_world);
	}

	DEFINE_BUFFER(buffer, buf);

	int mpi_status;

	for(int i=0; i<job_list[dest].worker_count; i++) {
		//printf("[Rank: %d] Job List: %d\n", node.rank, (job_list[dest].rank_list)[i]);
		debug_log_i("SEND: Data: %d to %d", *((int *)buf), (job_list[dest].rank_list)[i]);
		mpi_status = PMPI_Send(SET_RIGHT_S_BUFFER(buffer), count, datatype, (job_list[dest].rank_list)[i], tag, *comm_to_use);

		if(mpi_status != MPI_SUCCESS) {
			debug_log_i("MPI_Send Failed [Dest: %d]", (job_list[dest].rank_list)[i]);
		}
		else {
			debug_log_i("MPI_Send Success [Dest: %d]", (job_list[dest].rank_list)[i]);
		}
	}

	__ignore_process_failure = 0;

	release_comm_lock();

	debug_log_i("Still Alive");

	return mpi_status;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
	debug_log_i("In MPI_Recv()");
	is_file_update_set();
	acquire_comm_lock();

	__ignore_process_failure = 1;

	//int sender = 0;
	int mpi_status;
	MPI_Comm *comm_to_use;

	if(comm == MPI_COMM_WORLD) {
		//PMPI_Comm_dup(node.rep_mpi_comm_world, &comm_to_use);

		comm_to_use = &(node.rep_mpi_comm_world);
	}

	// DEBUG
	/*int rank;
	PMPI_Comm_rank(*comm_to_use, &rank);
	debug_log_i("This rank: %d", rank);
	
	DEFINE_BUFFER(buffer, buf);
	mpi_status = PMPI_Recv(SET_RIGHT_R_BUFFER(buffer), count, datatype, (job_list[source].rank_list)[0], tag, *comm_to_use, status);
	debug_log_i("RECV: Data: %d", *((int *)buf));

	if(mpi_status != MPI_SUCCESS) {
		debug_log_i("MPI_Recv Failed [Dest: %d]", (job_list[source].rank_list)[0]);
		//sender++;
	}
	else {
		debug_log_i("MPI_Recv Success [Dest: %d]", (job_list[source].rank_list)[0]);
	}*/

	DEFINE_BUFFER(buffer, buf);

	for(int i=0; i<job_list[source].worker_count; i++) {
		mpi_status = PMPI_Recv(SET_RIGHT_R_BUFFER(buffer), count, datatype, (job_list[source].rank_list)[i], tag, *comm_to_use, status);

		if(mpi_status != MPI_SUCCESS) {
			debug_log_i("MPI_Recv Failed [Dest: %d]", (job_list[source].rank_list)[i]);
			//sender++;
		}
		else {
			debug_log_i("MPI_Recv Success [Dest: %d]", (job_list[source].rank_list)[i]);
		}
	}

	__ignore_process_failure = 0;

	release_comm_lock();

	return mpi_status;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
	debug_log_i("MPI_Scatter Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	int total_trails = 0;
	MPI_Comm *comm_to_use;

	is_file_update_set();
	acquire_comm_lock();

	DEFINE_BUFFER(sbuffer, sendbuf);
	DEFINE_BUFFER(rbuffer, recvbuf);

	if(comm == MPI_COMM_WORLD) {
		comm_to_use = &(node.rep_mpi_comm_world);
	}

	do {

		mpi_status = MPI_SUCCESS;

		total_trails++;

		if(total_trails >= NO_TRIALS) {
			log_e("Total Trails exceeds. Aborting.");
			PMPI_Abort(node.rep_mpi_comm_world, 10);
		}

		int pp;
		PMPI_Comm_rank(*comm_to_use, &pp);
		debug_log_i("Comm_to_use rank: %d", pp);

		if(node.active_comm != MPI_COMM_NULL) {
			int ll;
			PMPI_Comm_size(node.active_comm, &ll);
			debug_log_i("node.active_comm Size: %d", ll);

			debug_log_i("RIGHT BUFFER: %d", SET_RIGHT_S_BUFFER(sbuffer), SET_RIGHT_R_BUFFER(rbuffer));
			mpi_status = PMPI_Scatter(SET_RIGHT_S_BUFFER(sbuffer), sendcount, sendtype, SET_RIGHT_R_BUFFER(rbuffer), recvcount, recvtype, root, node.active_comm);	
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		// why "while"? check MPI_Bcast.
		while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			//flag = 0;
			//continue;
		}
		// To perform agree on flag
		//PMPIX_Comm_agree(comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Scatter Failed");
			flag = 0;
			continue;
		}
		else {

			/*if(node.rank == 1)
				raise(SIGKILL);*/
			
			do {

				debug_log_i("Doing Bcast");

				
				mpi_status = PMPI_Bcast(SET_RIGHT_R_BUFFER(rbuffer), recvcount, recvtype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag));
				// To perform agree on flag
				//PMPIX_Comm_agree(comm_to_use, &flag);

				debug_log_i("[Value flag]: %d", flag);
				if(!flag) {
					debug_log_i("MPI_Bcast Failed");

					// If failed node is root node in node.world_job_comm, data is lost 
					// and main operation needs to be done again before doing bcast.
					if(is_failed_node_world_job_comm_root()) {
						debug_log_i("node.world_job_comm root node died. Doing main communication again");
						flag = 0;
						break;
					}
				}

			} while(!flag);
			
		}

		

	} while(!flag);

	release_comm_lock();

	return MPI_SUCCESS;
}

int MPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

	int mpi_status = MPI_SUCCESS;
	int flag;
	int total_trails = 0;
	MPI_Comm *comm_to_use;

	is_file_update_set();
	acquire_comm_lock();

	// Hack to pass pointers
	DEFINE_BUFFER(buffer, buf);

	if(comm == MPI_COMM_WORLD) {
		comm_to_use = &(node.rep_mpi_comm_world);
	}

	do {
		total_trails++;

		if(total_trails >= NO_TRIALS) {
			log_e("Total Trails exceeds. Aborting.");
			PMPI_Abort(node.rep_mpi_comm_world, 10);
		}

		debug_log_i("Starting bcast: Comm: %p | node.rep_comm: %p | Comm to use: %p", comm, node.rep_mpi_comm_world, *comm_to_use);

		int root_rank = job_list[root].rank_list[0];


		mpi_status = PMPI_Bcast(SET_RIGHT_S_BUFFER(buffer), count, datatype, root_rank, *comm_to_use);


		flag = (MPI_SUCCESS == mpi_status);

		debug_log_i("bcast done Success: %d", flag);

		// To correct the comms [Refer to Issue #29 on Github]
		// while loop should only run twice:
		//  1. To correct (shrink) the comm incase of failure
		//  2. To agree on flag value
		while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			//flag = 0;
			//continue; 	// This was a bad idea
		}
		// To perform agree on flag
		//PMPIX_Comm_agree(comm_to_use, &flag);  	// Initial thinking was correct

		if(!flag) {
			debug_log_i("MPI_Bcast Failed");
			flag = 0;
			continue;
		}

	} while(!flag);

	release_comm_lock();

	return MPI_SUCCESS;


	/*int color = 1;
	int rank_order = 1;
	MPI_Comm bcast_comm;

	if(node.job_id == root) {
		if(node.node_checkpoint_master == NO) {
			color = 0;
		}
	}
	else {
		rank_order = 10;
	}

	// Is comm split an efficient method.
	// What if replica node is allowed to receive data even though it already has it.
	// Will it be more efficient as compared to creating a new comm for Bcast?

	// Found something.
	// This would require all nodes to know rank of sending node. This we have to send to all the nodes.
	// This is an overhead. Its better to split and remove replica before. Like done here.
	PMPI_Comm_split(comm, color, rank_order, &bcast_comm);

	if(color == 1) {
		return PMPI_Bcast(buffer, count, datatype, 0, bcast_comm);
	}*/
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
	
	debug_log_i("MPI_Gather Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	int total_trails = 0;
	MPI_Comm *comm_to_use;

	is_file_update_set();
	acquire_comm_lock();

	DEFINE_BUFFER(sbuffer, sendbuf);
	DEFINE_BUFFER(rbuffer, recvbuf);
	
	if(comm == MPI_COMM_WORLD) {
		comm_to_use = &(node.rep_mpi_comm_world);
	}

	do {

		total_trails++;

		if(total_trails >= NO_TRIALS) {
			log_e("Total Trails exceeds. Aborting.");
			PMPI_Abort(node.rep_mpi_comm_world, 10);
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Gather(SET_RIGHT_S_BUFFER(sbuffer), sendcount, sendtype, SET_RIGHT_R_BUFFER(rbuffer), recvcount, recvtype, root, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			//flag = 0;
			//continue;
		}
		// To perform agree on flag
		//PMPIX_Comm_agree(comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Gather Failed");
			flag = 0;
			continue;
		}
		else {

			//if(node.rank == 1)
				//raise(SIGKILL);
			
			do {

				debug_log_i("Doing Bcast");
				mpi_status = PMPI_Bcast(SET_RIGHT_R_BUFFER(rbuffer), recvcount * node.jobs_count, recvtype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag));
				// To perform agree on flag
				//PMPIX_Comm_agree(comm_to_use, &flag);	// TODO: Is this call even required?

				debug_log_i("[Value flag]: %d", flag);
				if(!flag) {
					debug_log_i("MPI_Bcast Failed");

					// If failed node is root node in node.world_job_comm, data is lost 
					// and main operation needs to be done again before doing bcast.
					if(is_failed_node_world_job_comm_root()) {
						debug_log_i("node.world_job_comm root node died. Doing main communication again");
						flag = 0;
						break;
					}
				}

			} while(!flag);
			
		}

		

	} while(!flag);

	release_comm_lock();

	return MPI_SUCCESS;



	/*if(node.active_comm != MPI_COMM_NULL) {
		PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, node.active_comm);
	}

	// TODO: if only 1 node in world_job_comm, dont do bcast.
	if(node.job_id == root) {
		PMPI_Bcast(recvbuf, recvcount * node.jobs_count, recvtype, 0, node.world_job_comm);
	}

	return MPI_SUCCESS;*/

	/*int dsize;

    if(node.job_id == root) {
    	MPI_Type_size(sendtype, &dsize);
       	for(int i=0; i<node.jobs_count; i++) {
            if(root == i)
                continue;
            MPI_Recv(recvbuf + recvcount * i * dsize, recvcount, recvtype, i, 13, comm, MPI_STATUS_IGNORE);
        }
        memcpy(recvbuf + recvcount * root * dsize, sendbuf, dsize * recvcount);
    }
    else {
        MPI_Send(sendbuf, sendcount, sendtype, root, 13, comm);
    }

    return MPI_SUCCESS;*/
    
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
		
	debug_log_i("MPI_Allgather Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	int total_trails = 0;
	MPI_Comm *comm_to_use;

	is_file_update_set();
	acquire_comm_lock();

	DEFINE_BUFFER(sbuffer, sendbuf);
	DEFINE_BUFFER(rbuffer, recvbuf);

	if(comm == MPI_COMM_WORLD) {
		comm_to_use = &(node.rep_mpi_comm_world);
	}

	do {

		total_trails++;

		if(total_trails >= NO_TRIALS) {
			log_e("Total Trails exceeds. Aborting.");
			PMPI_Abort(node.rep_mpi_comm_world, 10);
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Allgather(SET_RIGHT_S_BUFFER(sbuffer), sendcount, sendtype, SET_RIGHT_R_BUFFER(rbuffer), recvcount, recvtype, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			//flag = 0;
			//continue;
		}
		// To perform agree on flag
		//PMPIX_Comm_agree(*comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Allgather Failed");
			flag = 0;
			continue;
		}
		else {

			/*if(node.rank == 1)
				raise(SIGKILL);*/
			
			do {

				debug_log_i("Doing Bcast");
				mpi_status = PMPI_Bcast(SET_RIGHT_R_BUFFER(rbuffer), recvcount * node.jobs_count, recvtype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag));
				// To perform agree on flag
				//PMPIX_Comm_agree(comm_to_use, &flag);

				debug_log_i("[Value flag]: %d", flag);
				if(!flag) {
					debug_log_i("MPI_Bcast Failed");

					// If failed node is root node in node.world_job_comm, data is lost 
					// and main operation needs to be done again before doing bcast.
					if(is_failed_node_world_job_comm_root()) {
						debug_log_i("node.world_job_comm root node died. Doing main communication again");
						flag = 0;
						break;
					}
				}

			} while(!flag);
			
		}

		

	} while(!flag);

	release_comm_lock();

	return MPI_SUCCESS;



	/*if(node.active_comm != MPI_COMM_NULL) {
		PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, node.active_comm);
	}

	if(job_list[node.job_id].worker_count > 1) {
		int rank;
		PMPI_Comm_rank(node.world_job_comm, &rank);
		debug_log_i("job_list[node.job_id].worker_count: %d | Rank: %d", job_list[node.job_id].worker_count, rank);
		PMPI_Bcast(recvbuf, recvcount * node.jobs_count, recvtype, 0, node.world_job_comm);
	}
	
	return MPI_SUCCESS;*/



/*
	MPI_Comm gather_comm;
	int color = 1;
	int send_rank = node.rank;
	int sync_rank;

	if(node.node_checkpoint_master == NO) {
		color = 0;
		send_rank = 99999;
	}

	PMPI_Allreduce(&send_rank, &sync_rank, 1, MPI_INT, MPI_MIN, comm);

	PMPI_Comm_split(comm, color, 0, &gather_comm);

	if(color == 1) {
		PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, gather_comm);
	}

	return PMPI_Bcast(recvbuf, recvcount * node.jobs_count, recvtype, sync_rank, comm);*/
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
	
	debug_log_i("MPI_Reduce Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	int total_trails = 0;
	MPI_Comm *comm_to_use;

	is_file_update_set();
	acquire_comm_lock();

	DEFINE_BUFFER(sbuffer, sendbuf);
	DEFINE_BUFFER(rbuffer, recvbuf);

	if(comm == MPI_COMM_WORLD) {
		comm_to_use = &(node.rep_mpi_comm_world);
	}

	do {

		total_trails++;

		if(total_trails >= NO_TRIALS) {
			log_e("Total Trails exceeds. Aborting.");
			PMPI_Abort(node.rep_mpi_comm_world, 10);
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Reduce(SET_RIGHT_S_BUFFER(sbuffer), SET_RIGHT_R_BUFFER(rbuffer), count, datatype, op, root, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			//flag = 0;
			//continue;
		}
		// To perform agree on flag
		//PMPIX_Comm_agree(*comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Reduce Failed");
			flag = 0;
			continue;
		}
		else {

			/*if(node.rank == 1)
				raise(SIGKILL);*/
			
			do {
				if(node.job_id == root) {
					debug_log_i("Doing Bcast");
					mpi_status = PMPI_Bcast(SET_RIGHT_R_BUFFER(rbuffer), count, datatype, 0, node.world_job_comm);
					debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				}
				else {
					mpi_status = MPI_SUCCESS;
				}
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag));
				// To perform agree on flag
				//PMPIX_Comm_agree(*comm_to_use, &flag);

				debug_log_i("[Value flag]: %d", flag);
				if(!flag) {
					debug_log_i("MPI_Bcast Failed");

					// If failed node is root node in node.world_job_comm, data is lost 
					// and main operation needs to be done again before doing bcast.
					if(is_failed_node_world_job_comm_root()) {
						debug_log_i("node.world_job_comm root node died. Doing main communication again");
						flag = 0;
						break;
					}
				}

			} while(!flag);
			
		}

		

	} while(!flag);

	release_comm_lock();

	return MPI_SUCCESS;




	/*if(node.active_comm != MPI_COMM_NULL) {
		PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, node.active_comm);
		if(node.job_id == root)
			debug_log_i("OP Value: %d", *((int *)recvbuf));
	}

	if(node.job_id == root && job_list[node.job_id].worker_count > 1) {
		//int rank;
		//PMPI_Comm_rank(node.world_job_comm, &rank);
		//debug_log_i("job_list[node.job_id].worker_count: %d | Rank: %d", job_list[node.job_id].worker_count, rank);
		PMPI_Bcast(recvbuf, count, datatype, 0, node.world_job_comm);
	}
	
	return MPI_SUCCESS;*/
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
	
	debug_log_i("MPI_Allreduce Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	int total_trails = 0;
	MPI_Comm *comm_to_use;

	is_file_update_set();
	acquire_comm_lock();

	DEFINE_BUFFER(sbuffer, sendbuf);
	DEFINE_BUFFER(rbuffer, recvbuf);
	
	if(comm == MPI_COMM_WORLD) {
		comm_to_use = &(node.rep_mpi_comm_world);
	}

	do {

		total_trails++;

		if(total_trails >= NO_TRIALS) {
			log_e("Total Trails exceeds. Aborting.");
			PMPI_Abort(node.rep_mpi_comm_world, 10);
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Allreduce(SET_RIGHT_S_BUFFER(sbuffer), SET_RIGHT_R_BUFFER(rbuffer), count, datatype, op, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			//flag = 0;
			//continue;
		}
		// To perform agree on flag
		//PMPIX_Comm_agree(*comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Allreduce Failed");
			flag = 0;
			continue;
		}
		else {

			/*if(node.rank == 1)
				raise(SIGKILL);*/
			
			do {

				debug_log_i("Doing Bcast");
				mpi_status = PMPI_Bcast(SET_RIGHT_R_BUFFER(rbuffer), count, datatype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				while(MPI_SUCCESS != PMPIX_Comm_agree(*comm_to_use, &flag));
				// To perform agree on flag
				//PMPIX_Comm_agree(*comm_to_use, &flag);

				debug_log_i("[Value flag]: %d", flag);
				if(!flag) {
					debug_log_i("MPI_Bcast Failed");

					// If failed node is root node in node.world_job_comm, data is lost 
					// and main operation needs to be done again before doing bcast.
					if(is_failed_node_world_job_comm_root()) {
						debug_log_i("node.world_job_comm root node died. Doing main communication again");
						flag = 0;
						break;
					}
				}

			} while(!flag);
			
		}

		

	} while(!flag);

	release_comm_lock();

	return MPI_SUCCESS;


	/*if(node.active_comm != MPI_COMM_NULL) {
		PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, node.active_comm);
	}

	if(job_list[node.job_id].worker_count > 1) {
		// int rank;
		// PMPI_Comm_rank(node.world_job_comm, &rank);
		// debug_log_i("job_list[node.job_id].worker_count: %d | Rank: %d", job_list[node.job_id].worker_count, rank);
		PMPI_Bcast(recvbuf, count, datatype, 0, node.world_job_comm);
	}
	
	return MPI_SUCCESS;*/
}

// Non blocking MPI_I* calls

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
	debug_log_i("In MPI_Isend()");
	is_file_update_set();
	acquire_comm_lock();
	debug_log_i("In MPI_Isend() after is_file_update_set");

	__ignore_process_failure = 1;

	MPI_Comm *comm_to_use;

	if(comm == MPI_COMM_WORLD) {
		// So that comm err handler do not identify it as rep_mpi_comm_world
		// It will ignore it and will not enter the if statement.
		// Note: If it enters, process will hang as functions inside are
		// collective.
		//PMPI_Comm_dup(node.rep_mpi_comm_world, &comm_to_use);

		comm_to_use = &(node.rep_mpi_comm_world);
	}

	DEFINE_BUFFER(buffer, buf);

	int mpi_status;

	Aggregate_Request *agg_req = new_agg_request();

	// "request" variable will now contain the address of the aggregated request
	// element instead of MPI_Request.
	*request = (MPI_Request)agg_req;

	for(int i=0; i<job_list[dest].worker_count; i++) {
		//printf("[Rank: %d] Job List: %d\n", node.rank, (job_list[dest].rank_list)[i]);
		debug_log_i("SEND: Data: %d", *((int *)buf));

		MPI_Request *r = add_new_request(agg_req);

		mpi_status = PMPI_Isend(SET_RIGHT_S_BUFFER(buffer), count, datatype, (job_list[dest].rank_list)[i], tag, *comm_to_use, r);

		if(mpi_status != MPI_SUCCESS) {
			debug_log_i("MPI_Isend Failed [Dest: %d]", (job_list[dest].rank_list)[i]);
		}
		else {
			debug_log_i("MPI_Isend Success [Dest: %d]", (job_list[dest].rank_list)[i]);
		}
	}

	__ignore_process_failure = 0;

	release_comm_lock();

	return mpi_status;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
	debug_log_i("In MPI_Irecv()");
	is_file_update_set();
	acquire_comm_lock();

	__ignore_process_failure = 1;

	//int sender = 0;
	int mpi_status;
	MPI_Comm *comm_to_use;

	if(comm == MPI_COMM_WORLD) {
		//PMPI_Comm_dup(node.rep_mpi_comm_world, &comm_to_use);

		comm_to_use = &(node.rep_mpi_comm_world);
	}

	DEFINE_BUFFER(buffer, buf);

	Aggregate_Request *agg_req = new_agg_request();

	// "request" variable will now contain the address of the aggregated request
	// element instead of MPI_Request.
	*request = (MPI_Request)agg_req;

	int worker_count;

	if(source == MPI_ANY_SOURCE) {
		// Very tricky to set.
		// If less and user sends more MPI_Send it will block the program.
		// TODO: Find a better solution for MPI_ANY_SOURCE
		worker_count = 2;
	} else {
		worker_count = job_list[source].worker_count;
	}

	for(int i=0; i<worker_count; i++) {

		int recv_source;
		recv_source = (source == MPI_ANY_SOURCE) ? MPI_ANY_SOURCE : (job_list[source].rank_list)[i];
		/*if(source == MPI_ANY_SOURCE) {
			recv_source = MPI_ANY_SOURCE;
		} else {
			recv_source = (job_list[source].rank_list)[i];
		}*/

		MPI_Request *r = add_new_request(agg_req);
		debug_log_i("*******REQUEST BEFORE: %p | Source: %d", *r, recv_source);
		mpi_status = PMPI_Irecv(SET_RIGHT_R_BUFFER(buffer), count, datatype, recv_source, tag, *comm_to_use, r);
		debug_log_i("*******REQUEST AFTER: %p", *r);

		if(mpi_status != MPI_SUCCESS) {
			debug_log_i("MPI_Irecv Failed [Source: %d]", recv_source);
			//sender++;
		}
		else {
			debug_log_i("MPI_Irecv Success [Source: %d]", recv_source);
		}
	}

	__ignore_process_failure = 0;

	release_comm_lock();

	return mpi_status;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
	debug_log_i("In MPI_Wait()");
	return wait_for_agg_request(*request, status);
}
