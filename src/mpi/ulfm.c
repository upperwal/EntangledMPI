#include "ulfm.h"

extern Job *job_list;
extern Node node;
extern int *rank_2_job;

extern int __ignore_process_failure;

MPI_Group last_group_failed;	// This is the group of nodes which failed recently.
MPI_Group previous_world_group;

MPI_Comm world_shrinked;
MPI_Group group_world_dup;
MPI_Group group_world_shrinked;

void update_job_list(int size, int *translated_ranks) {
	node.node_checkpoint_master = NO;

	for(int i=0; i<node.jobs_count; i++) {
		
		int overwrite_pointer = 0;
		int worker_count = job_list[i].worker_count;
		for(int j=0; j<worker_count; j++) {

			int rank = (job_list[i].rank_list)[j];
			assert(rank < size && "Stored rank should be less than comm size.");

			if(translated_ranks[rank] == MPI_UNDEFINED) {
				job_list[i].worker_count--;

				if(job_list[i].worker_count == 0) {
					log_e("Job: %d has no living worker node.", i);
					PMPI_Abort(node.rep_mpi_comm_world, 200);
				}
			}
			else {
				(job_list[i].rank_list)[overwrite_pointer++] = translated_ranks[rank];
				rank_2_job[ translated_ranks[rank] ] = i;
			}
		
		}
	}

	node.rank = translated_ranks[node.rank];

	if((job_list[node.job_id].rank_list)[0] == node.rank) {
		node.node_checkpoint_master = YES;
	}
	debug_log_i("update job Checkpoint MASTER: %d", node.node_checkpoint_master == YES);
}

void rep_errhandler(MPI_Comm* pcomm, int* perr, ...) {
    
    int err = *perr, eclass, compare_result, err_len;
    char err_string[100];

	PMPI_Error_class(err, &eclass);

	debug_log_i("Comm: %p | world: %p | Error: %d | Is W: %d | J: %d | A: %d", *pcomm, node.rep_mpi_comm_world, eclass == MPIX_ERR_PROC_FAILED, *pcomm == node.rep_mpi_comm_world, *pcomm == node.world_job_comm, *pcomm == node.active_comm);

	if(MPIX_ERR_PROC_FAILED != eclass) {
		PMPI_Error_string(err, err_string, &err_len);
		log_e("Error: %s", err_string);
		PMPI_Abort(node.rep_mpi_comm_world, 300);
	}

	PMPI_Comm_compare(*pcomm, node.rep_mpi_comm_world, &compare_result);
	debug_log_i("MPI_IDENT: %d | MPI_CONGRUENT: %d | MPI_SIMILAR: %d | MPI_UNEQUAL: %d", MPI_IDENT == compare_result, MPI_CONGRUENT == compare_result, MPI_SIMILAR == compare_result, MPI_UNEQUAL == compare_result);
	if(*pcomm == node.rep_mpi_comm_world && MPIX_ERR_PROC_FAILED == eclass && !__ignore_process_failure) {

		int *rank_arr_group_world_dup, *rank_arr_group_world_shrinked;
		int size;

		PMPIX_Comm_failure_ack(node.rep_mpi_comm_world);
    	PMPIX_Comm_failure_get_acked(node.rep_mpi_comm_world, &last_group_failed);

		debug_log_i("Shrinking World Comm.");
		PMPIX_Comm_shrink(*pcomm, &world_shrinked);

		debug_log_i("Extract World Group");
		PMPI_Comm_group(*pcomm, &group_world_dup);
		PMPI_Comm_group(world_shrinked, &group_world_shrinked);

		previous_world_group = group_world_dup;

		PMPI_Group_size(group_world_dup, &size);
		rank_arr_group_world_dup = malloc(sizeof(int) * size);
		rank_arr_group_world_shrinked = malloc(sizeof(int) * size);

		debug_log_i("Comm size: %d", size);

		for(int i=0; i<size; i++) {
			rank_arr_group_world_dup[i] = i;
		}

		

		PMPI_Group_translate_ranks(group_world_dup, size, rank_arr_group_world_dup, group_world_shrinked, rank_arr_group_world_shrinked);

		for(int i=0; i<size; i++) {
			debug_log_i("Rank Transalation: %d", rank_arr_group_world_shrinked[i]);
		}

		update_job_list(size, rank_arr_group_world_shrinked);
		print_job_list();

		PMPI_Comm_free(&(node.rep_mpi_comm_world));
		PMPI_Comm_dup(world_shrinked, &(node.rep_mpi_comm_world));
		PMPI_Comm_free(&world_shrinked);
		//node.rep_mpi_comm_world = world_shrinked;

		if(node.active_comm != MPI_COMM_NULL) {
			PMPI_Comm_free(&(node.active_comm));
		}
		PMPI_Comm_free(&(node.world_job_comm));
		
		update_comms();
	}

    return;
    /*MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, size, nf, len, eclass;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    PMPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        PMPI_Abort(comm, err);
    }

    PMPI_Comm_rank(comm, &rank);
    PMPI_Comm_size(comm, &size);

    // We use a combination of 'ack/get_acked' to obtain the list of 
    // failed processes (as seen by the local rank). 
    PMPIX_Comm_failure_ack(comm);
    PMPIX_Comm_failure_get_acked(comm, &group_f);
    PMPI_Group_size(group_f, &nf);
    PMPI_Error_string(err, errstr, &len);
    debug_log_i("Rank %d / %d: Notified of error %s. %d found dead: ",
           rank, size, errstr, nf);

    // We use 'translate_ranks' to obtain the ranks of failed procs 
    // in the input communicator 'comm'.
    // 
    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    PMPI_Comm_group(comm, &group_c);
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    PMPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);
    for(i = 0; i < nf; i++)
        debug_log_i("%d ", ranks_gc[i]);
    free(ranks_gf); free(ranks_gc);*/
}

// This function returns true (1) if the died node is root node in node.world_job_comm.
// Root node ie. rank 0 of node.world_job_comm is responsible for all collective calls
// and then it bcast it to all its replica or supportive nodes.
int is_failed_node_world_job_comm_root() {
	MPI_Group group_world;
	int failed_group_size;
	int *rank_failed_group, *rank_world_comm;

	/*PMPIX_Comm_failure_ack(node.rep_mpi_comm_world);
    PMPIX_Comm_failure_get_acked(node.rep_mpi_comm_world, &group_failed);*/

    PMPI_Group_size(last_group_failed, &failed_group_size);

    //PMPI_Comm_group(node.rep_mpi_comm_world, &group_world);

    rank_failed_group = malloc(sizeof(int) * failed_group_size);
    rank_world_comm = malloc(sizeof(int) * failed_group_size);

    for(int i=0; i<failed_group_size; i++) {
    	rank_failed_group[i] = i;
    }

    PMPI_Group_translate_ranks(last_group_failed, failed_group_size, rank_failed_group, previous_world_group, rank_world_comm);

    debug_log_i("Number of node died: %d", failed_group_size);
    for(int j=0; j<failed_group_size; j++) {
    	debug_log_i("Died Nodes: %d", rank_world_comm[j]);
    }

    for(int i=0; i<node.jobs_count; i++) {
		
		for(int j=0; j<failed_group_size; j++) {
			if((job_list[i].rank_list)[0] == rank_world_comm[j])
				return 1;
		}
	}

	return 0;
}
