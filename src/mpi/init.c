#include <mpi.h>

#ifdef OPEN_MPI
#include <mpi-ext.h>
#endif

#include <stdio.h>
#include <signal.h>

#include "src/shared.h"
#include "src/replication/rep.h"
#include "src/misc/file.h"
#include "src/mpi/comm.h"
#include "src/checkpoint/full_context.h"
#include "src/mpi/ulfm.h"

#define REP_THREAD_SLEEP_TIME 3

jmp_buf context;
address stackHigherAddress, stackLowerAddress;
enum MapStatus map_status;

/* pthread mutex */
pthread_mutex_t global_mutex;
pthread_mutex_t rep_time_mutex;

Job *job_list;
Node node;

char *map_file = "./replication.map";
char *ckpt_file = "./ckpt/rank-%d.ckpt";

// Restore from checkpoint files: YES | Do not restore: NO
enum CkptBackup ckpt_backup;

// this comm contains sender and receiver nodes during replication.
MPI_Comm job_comm;

MPI_Errhandler ulfm_err_handler;

extern Malloc_list *head;

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
	PMPI_Init(argc, argv);



	PMPI_Comm_create_errhandler(rep_errhandler, &ulfm_err_handler);
	//PMPI_Comm_set_errhandler(MPI_COMM_WORLD, ulfm_err_handler);

	PMPI_Comm_dup(MPI_COMM_WORLD, &(node.rep_mpi_comm_world));
	PMPI_Comm_set_errhandler(node.rep_mpi_comm_world, ulfm_err_handler);

	// job_list and node declared in shared.h
	init_node(map_file, &job_list, &node);

	debug_log_i("Initialising MPI.");
	debug_log_i("Node variable address: %p", &node);
	
	if(argv == NULL) {
		debug_log_e("You must pass &argv to MPI_Init()");
		exit(0);
	}

	debug_log_i("WORLD_COMM Dup: %p", node.rep_mpi_comm_world);

	address stackStart;

	// Getting RBP only works if optimisation level is zero (O0).
	// O1 removes RBP's use.
	/*Init_Rep(stackStart);	// a macro to fetch RBP of main function.
	if(stackStart == 0) {
		// OS kept values of program arguments this this address which is below main function frame.
		// Writable to the user and same for each program. No harm to overwrite this data as it is same for
		// all the nodes. [Same arguments passed]
		stackStart = **argv;
	}*/

	stackStart = *argv;

	// Lock global mutex. This mutex will always be locked when user program is executing.
	pthread_mutex_init(&global_mutex, NULL);
	pthread_mutex_init(&rep_time_mutex, NULL);
	
	pthread_mutex_lock(&global_mutex);

	debug_log_i("Address Stack new: %p", stackStart);

	ckpt_bit = does_ckpt_file_exists(ckpt_file);

	if(ckpt_bit) {
		ckpt_backup = BACKUP_YES;
	}

	pthread_t tid;
	pthread_create(&tid, NULL, rep_thread_init, stackStart);

	if(ckpt_bit) {
		while(is_file_update_set() != 1);
	}

	PMPI_Barrier(node.rep_mpi_comm_world);
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
	debug_log_i("In MPI_Send() after is_file_update_set");

	/*int sender = 0;
	int mpi_status;

	//while(sender < job_list[node.job_id].worker_count) {

		
	for(int i=0; i<job_list[dest].worker_count; i++) {
		//printf("[Rank: %d] Job List: %d\n", node.rank, (job_list[dest].rank_list)[i]);
		
		if(node.rank == (job_list[node.job_id].rank_list)[sender]) {
			debug_log_i("SEND: Data: %d", *((int *)buf));
			mpi_status = PMPI_Send(buf, count, datatype, (job_list[dest].rank_list)[i], tag, comm);
			
			// If one of the receiving process/node fails, ignore it as its replica will
			// receive the data or if no replica, abort will happen.
			if(mpi_status == MPIX_ERR_PROC_FAILED) {
				debug_log_i("MPI_Send Failed [Dest: %d]", (job_list[dest].rank_list)[i]);
			}
			else {
				debug_log_i("MPI_Send Success [Dest: %d]", (job_list[dest].rank_list)[i]);
			}
		}

		debug_log_i("Before Send barrier");

		// This barrier is to check if any node within this job has died.
		// If yes, we assume previous MPI_Send was also a failure so try 
		// to send it again.

		// If MPI_Send was a success and MPI_Barrier fails, there would be no
		// corresponding MPI_Recv if we try MPI_Send again. This could be a 
		// PROBLEM.
		mpi_status = PMPI_Barrier(node.world_job_comm);



		debug_log_i("After Send barrier | Is status: %d | MPI_SUCCESS: %d", mpi_status, MPI_SUCCESS);
		//MPIX_Comm_failure_ack(node.world_job_comm);
		
		//ulfm_detect(mpi_status);

		// if this status is not MPI_SUCCESS a sender node has died.
		// TODO: What if replica died. This will increment the sender which is incorrect.
		if(mpi_status == MPI_ERR_PROC_FAILED) {
			debug_log_i("Died Rank: %d", (job_list[node.job_id].rank_list)[sender]);
			MPI_Comm new_job_comm;

			mpi_status = PMPIX_Comm_shrink(node.world_job_comm, &new_job_comm);
			

			// DEBUG CODE
			char str[500];
			int len;

			if (mpi_status) {
				int errclass;
				MPI_Error_class(mpi_status, &errclass);
				MPI_Error_string(mpi_status, str, &len);
				debug_log_i("Expected MPI_SUCCESS from MPIX_Comm_shrink. Received: %s\n", str);
				//errs++;
				MPI_Abort(node.rep_mpi_comm_world, 1);
			}
			// DEBUG CODE ENDS


			int sz;
			int rk;
			PMPI_Comm_rank(new_job_comm, &rk);
			PMPI_Comm_size(new_job_comm, &sz);
			debug_log_i("Shrink Comm Rank: %d | Size: %d", rk, sz);
			
			PMPI_Comm_free(node.world_job_comm);
			node.world_job_comm = new_job_comm;
			
			i--;
			sender++;
		}
		else {
			debug_log_i("Rank: %d able to send to all", (job_list[node.job_id].rank_list)[sender]);
			//break;
		}
	}*/

	MPI_Comm comm_to_use;

	if(comm == MPI_COMM_WORLD) {
		comm_to_use = node.rep_mpi_comm_world;
	}

	// Not fault tolerant
	if(node.node_checkpoint_master == YES) {
		for(int i=0; i<job_list[dest].worker_count; i++) {
			//printf("[Rank: %d] Job List: %d\n", node.rank, (job_list[dest].rank_list)[i]);
			debug_log_i("SEND: Data: %d", *((int *)buf));
			int mpi_status = PMPI_Send(buf, count, datatype, (job_list[dest].rank_list)[i], tag, comm_to_use);
			
			if(mpi_status != MPI_SUCCESS) {
				debug_log_i("MPI_Send Failed [Dest: %d]", (job_list[dest].rank_list)[i]);
			}
			else {
				debug_log_i("MPI_Send Success [Dest: %d]", (job_list[dest].rank_list)[i]);
			}
		}
	}

	return MPI_SUCCESS;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
	debug_log_i("In MPI_Recv()");
	is_file_update_set();

	//int sender = 0;
	int mpi_status;
	MPI_Comm comm_to_use;

	if(comm == MPI_COMM_WORLD) {
		comm_to_use = node.rep_mpi_comm_world;
	}

	mpi_status = PMPI_Recv(buf, count, datatype, (job_list[source].rank_list)[0], tag, comm_to_use, status);
	debug_log_i("RECV: Data: %d", *((int *)buf));

	if(mpi_status != MPI_SUCCESS) {
		debug_log_i("MPI_Recv Failed [Dest: %d]", (job_list[source].rank_list)[0]);
		//sender++;
	}
	else {
		debug_log_i("MPI_Recv Success [Dest: %d]", (job_list[source].rank_list)[0]);
	}

	return MPI_SUCCESS;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
	debug_log_i("MPI_Scatter Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	MPI_Comm comm_to_use;

	is_file_update_set();

	

	do {

		if(comm == MPI_COMM_WORLD) {
			comm_to_use = node.rep_mpi_comm_world;
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, node.active_comm);	
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			flag = 0;
			continue;
		}
		// To perform agree on flag
		PMPIX_Comm_agree(comm_to_use, &flag);

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
				mpi_status = PMPI_Bcast(recvbuf, recvcount, recvtype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
					flag = 0;
				}
				// To perform agree on flag
				PMPIX_Comm_agree(comm_to_use, &flag);

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

	return MPI_SUCCESS;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
	
	debug_log_i("MPI_Gather Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	MPI_Comm comm_to_use;

	is_file_update_set();

	

	do {

		if(comm == MPI_COMM_WORLD) {
			comm_to_use = node.rep_mpi_comm_world;
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			flag = 0;
			continue;
		}
		// To perform agree on flag
		PMPIX_Comm_agree(comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Gather Failed");
			flag = 0;
			continue;
		}
		else {

			/*if(node.rank == 1)
				raise(SIGKILL);*/
			
			do {

				debug_log_i("Doing Bcast");
				mpi_status = PMPI_Bcast(recvbuf, recvcount * node.jobs_count, recvtype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
					flag = 0;
				}
				// To perform agree on flag
				PMPIX_Comm_agree(comm_to_use, &flag);	// TODO: Is this call even required?

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

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

	int mpi_status = MPI_SUCCESS;
	int flag;
	MPI_Comm comm_to_use;

	is_file_update_set();

	do {
		if(comm == MPI_COMM_WORLD) {
			comm_to_use = node.rep_mpi_comm_world;
		}

		debug_log_i("Starting bcast: Comm: %p | node.rep_comm: %p | Comm to use: %p", comm, node.rep_mpi_comm_world, comm_to_use);
		mpi_status = PMPI_Bcast(buffer, count, datatype, 0, comm_to_use);
		debug_log_i("bcast done");


		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			flag = 0;
			//continue;
		}
		// To perform agree on flag
		PMPIX_Comm_agree(comm_to_use, &flag);

		if(!flag) {
			debug_log_i("MPI_Bcast Failed");
			flag = 0;
			//continue;
		}

	} while(!flag);

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

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
		
	debug_log_i("MPI_Allgather Call");
	int mpi_status = MPI_SUCCESS;
	int flag;
	MPI_Comm comm_to_use;

	is_file_update_set();

	do {

		if(comm == MPI_COMM_WORLD) {
			comm_to_use = node.rep_mpi_comm_world;
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			flag = 0;
			continue;
		}
		// To perform agree on flag
		PMPIX_Comm_agree(comm_to_use, &flag);

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
				mpi_status = PMPI_Bcast(recvbuf, recvcount * node.jobs_count, recvtype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
					flag = 0;
				}
				// To perform agree on flag
				PMPIX_Comm_agree(comm_to_use, &flag);

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
	MPI_Comm comm_to_use;

	is_file_update_set();

	do {

		if(comm == MPI_COMM_WORLD) {
			comm_to_use = node.rep_mpi_comm_world;
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			flag = 0;
			continue;
		}
		// To perform agree on flag
		PMPIX_Comm_agree(comm_to_use, &flag);

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
					mpi_status = PMPI_Bcast(recvbuf, count, datatype, 0, node.world_job_comm);
					debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				}
				else {
					mpi_status = MPI_SUCCESS;
				}
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
					flag = 0;
				}
				// To perform agree on flag
				PMPIX_Comm_agree(comm_to_use, &flag);

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
	MPI_Comm comm_to_use;

	is_file_update_set();

	do {

		if(comm == MPI_COMM_WORLD) {
			comm_to_use = node.rep_mpi_comm_world;
		}

		if(node.active_comm != MPI_COMM_NULL) {
			mpi_status = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, node.active_comm);
		}

		flag = (MPI_SUCCESS == mpi_status);

		// To correct the comms
		if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
			debug_log_i("First Comm agree");
			flag = 0;
			continue;
		}
		// To perform agree on flag
		PMPIX_Comm_agree(comm_to_use, &flag);

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
				mpi_status = PMPI_Bcast(recvbuf, count, datatype, 0, node.world_job_comm);
				debug_log_i("Aftter bcast: MPI_Status: %d", mpi_status == MPI_SUCCESS);
				flag = (MPI_SUCCESS == mpi_status);
				
				// To correct the comms
				if(MPI_SUCCESS != PMPIX_Comm_agree(comm_to_use, &flag)) {
					flag = 0;
				}
				// To perform agree on flag
				PMPIX_Comm_agree(comm_to_use, &flag);

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
