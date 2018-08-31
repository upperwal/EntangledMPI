#include "async.h"

extern int *rank_2_job;

extern Node node;
extern Job *job_list;

extern void *__blackhole_address;
extern int __request_pending; 
extern int *rank_ignore_list;

//int r_count;
//MPI_Request *arr_request;
MPI_Status *arr_status;
MPI_Status wa_status;

int gdb_val = 0;

Aggregate_Request *new_agg_request(void *ori_buf, MPI_Datatype datatype, int count, int worker_count) {
	Aggregate_Request *agg_req = (Aggregate_Request *)malloc(sizeof(Aggregate_Request));

	int type_size;
	PMPI_Type_size(datatype, &type_size);

	//agg_req->list = NULL;
	//agg_req->buf_list = NULL;
	agg_req->request_count = worker_count;
	agg_req->original_buffer = ori_buf;
	agg_req->buffer_size = type_size * count;
	agg_req->async_type = NONE;
	agg_req->node_rank = (int *)malloc(sizeof(int) * worker_count);

	if(worker_count == 1) {
		agg_req->temp_buffer = ori_buf;
	} else {
		agg_req->temp_buffer = malloc(agg_req->buffer_size * worker_count);
	}

	__request_pending++;

	return agg_req;
}

/*MPI_Request *add_new_request_and_buffer(Aggregate_Request *agg, void **buf_add) {
	MPI_Request *request = (MPI_Request *)malloc(sizeof(MPI_Request));

	Request_List *req_list = (Request_List *)malloc(sizeof(Request_List));
	//req_list->req = request;
	req_list->next = agg->list;

	
	Buffer_List *buf_list = (Buffer_List *)malloc(sizeof(Buffer_List));
	buf_list->buf = (void *)malloc(agg->buffer_size);
	buf_list->next = agg->buf_list;
	*buf_add = buf_list->buf;

	agg->list = req_list;
	agg->buf_list = buf_list;
	//agg->request_count++;

	debug_log_i("Request Address Allocation: %p", request);

	return request;
}*/

int get_request_count(void *agg) {
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	return agg_req->request_count;
}

/*MPI_Request *get_array_of_request(void *agg, int *size) {
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;
	//log_i("AGG Req Add: %p", agg_req);

	*size = agg_req->request_count;

	if(*size == 0) {
		return NULL;
	}

	MPI_Request *array = (MPI_Request *)malloc(sizeof(MPI_Request) * agg_req->request_count);

	Request_List *list = agg_req->list;
	for(int i=0; i<*size; i++) {
		array[i] = *(list->req);

		debug_log_i("Request Address Usage: %p", list->req);

		if(list->next != NULL)
			list = list->next;

		// MPI_Status sta;

		// debug_log_i("Before LIST REQ: %d", *size);
		// PMPI_Wait(array + i, &sta);
		// debug_log_i("After LIST REQ");
	}

	return array;
}*/

int free_agg_request(void *agg) {
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	//Request_List *list = agg_req->list;
	//Buffer_List *buf_list = agg_req->buf_list;

	free(agg_req->req_array);
	if(agg_req->request_count > 1) {
		free(agg_req->temp_buffer);
	}
	free(agg_req->node_rank);
	free(agg_req);

	/*for(int i=1; i<agg_req->request_count; i++) {

		void *list_n = list->next;
		void *buf_n = buf_list->next;

		free(list->req);
		free(list);

		// freeing the buffer will result no place for other internal requests.
		// This will result in seg fault in MPI_Anywait.
		// TODO: Find a solution to this. To much memory wastage.
		//free(buf_list->buf);
		free(buf_list);

		// should check for both but its fine.
		if(list_n != NULL) {
			list = list_n;
			buf_list = buf_n;
		}
	}*/

	return MPI_SUCCESS;
}

int wait_for_agg_request(void *agg, MPI_Status *status) {
	
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	//arr_request = get_array_of_request(agg, &r_count);
	MPI_Request blackhole_request;

	if(agg_req->request_count == 0 || agg_req->req_array == NULL) {
		log_e("arr_request is NULL");
		// Should return something better.
		return MPI_SUCCESS;
	}

	int wa_index = 0;
	int result;

	if(status == MPI_STATUS_IGNORE) {
		//while(PMPI_Waitany(agg_req->request_count, agg_req->req_array, &wa_index, MPI_STATUSES_IGNORE));
		// TODO: Handle failure.
		int rc = PMPI_Waitall(agg_req->request_count, agg_req->req_array, MPI_STATUSES_IGNORE);

		free_agg_request(agg);

		return rc;
	}
	else {
		arr_status = (MPI_Status *)malloc(sizeof(MPI_Status) * agg_req->request_count);

		//PMPI_Wait(arr_request, arr_status);

		// MPI_Waitall is giving some wierd seg fault. 
		//while(PMPI_Waitall(r_count, arr_request, arr_status) != MPI_SUCCESS);
		
		

		// because we are ignoring process failure in send/recv
		// if a process failure occur below function will go crazy. Err handler will
		// be invoked again and again.
		// Could be optimized by correcting the job map and sending data to only alive nodes.
		// Other possibility would be to create an ignore map. Which include all failed nodes.
		// Added an ignore map.
		//int wstatus = PMPI_Waitany(r_count, arr_request, &wa_index, &wa_status);
		// If process dies during PMPI_Waitall it will return ~= MPI_SUCCESS
		// BUT this does not mean that PMPI_Waitall should be done again.
		// Actual status is within arr_status, check it and process accordingly.
		// Nodes without error will proceed fine and there is no need to do this calls again
		// IMPORTANT: Doing this call again will result in error because MPI_Request is
		// already exhausted and it will result undefined result.
		int success_from_atleast_one;
		int trial_counter = 0;
		do {
			debug_log_i("Wait attempt: %d", trial_counter+1);
			int wstatus = PMPI_Waitall(agg_req->request_count, agg_req->req_array, arr_status);
			debug_log_i("Wait attempt: %d over", trial_counter+1);
			success_from_atleast_one = 0;
			for(int i=0; i<agg_req->request_count; i++) {
				if(arr_status[i].MPI_SOURCE >= 0 && arr_status[i].MPI_ERROR == MPI_SUCCESS && rank_ignore_list[ arr_status[i].MPI_SOURCE ] != 1) {
					*status = arr_status[i];
					// check if arr_status[0].MPI_SOURCE < 0
					(*status).MPI_SOURCE = rank_2_job[arr_status[i].MPI_SOURCE];
					wa_index = i;
					success_from_atleast_one = 1;
					break;
				}
			}

			if(trial_counter++ > 4) {
				gdb_val = 2;
				log_e("NO MPI_SOURCE valid");
				exit(999);
			}

		} while(!success_from_atleast_one);
		debug_log_i("Found atleast one success in wait");
		//************************************************************************
		//int wstatus = PMPI_Waitall(agg_req->request_count, agg_req->req_array, arr_status);
		//************************************************************************

		//return MPI_SUCCESS;
		/*if(wstatus != MPI_SUCCESS) {
			log_i("waitany failed");
			gdb_val = 1;
			for(int i=0; i<agg_req->request_count; i++) {
				if(rank_ignore_list[ agg_req->node_rank[i] ] == 1) {
					log_i("Removing %d from request array", agg_req->node_rank[i]);
					agg_req->req_array[i] = MPI_REQUEST_NULL;
				}
			}
			
			wstatus = PMPI_Waitall(agg_req->request_count, agg_req->req_array, arr_status);
			wstatus = PMPI_Waitall(agg_req->request_count, agg_req->req_array, arr_status);
			wstatus = PMPI_Waitall(agg_req->request_count, agg_req->req_array, arr_status);
			if(wstatus != MPI_SUCCESS)
			       PMPI_Abort(MPI_COMM_WORLD, 400);

			log_i("Done wait any: SUCCESS: %d", wstatus == MPI_SUCCESS);
		}*/


		// TODO: while loop around PMPI_Waitany
		/*if(wstatus != MPI_SUCCESS) {
			log_i("waitany failed");

			wstatus = PMPI_Waitany(r_count, arr_request, &wa_index, &wa_status);
			if(wstatus != MPI_SUCCESS)
				PMPI_Abort(MPI_COMM_WORLD, 400);

			log_i("Done wait any");
		}*/

		/*for(int i=0; i<agg_req->request_count; i++) {
			if(arr_status[i].MPI_ERROR != MPI_SUCCESS) {
				(*status).MPI_SOURCE = rank_2_job[arr_status[i].MPI_SOURCE];
				(*status).MPI_TAG = arr_status[i].MPI_TAG;
				(*status).MPI_ERROR = arr_status[i].MPI_ERROR;

				return MPI_SUCCESS;
			}
		}*/

		//************************************************************************
		/*(*status).MPI_SOURCE = -1;
		for(int i=0; i<agg_req->request_count; i++) {
			if(arr_status[i].MPI_SOURCE >= 0 && arr_status[i].MPI_ERROR == MPI_SUCCESS && rank_ignore_list[ arr_status[i].MPI_SOURCE ] != 1) {
				*status = arr_status[i];
				// check if arr_status[0].MPI_SOURCE < 0
				(*status).MPI_SOURCE = rank_2_job[arr_status[i].MPI_SOURCE];
				wa_index = i;
				break;
			}
		}

		if((*status).MPI_SOURCE < 0) {
			gdb_val = 1;
			log_e("NO MPI_SOURCE valid");
			PMPI_Waitall(agg_req->request_count, agg_req->req_array, arr_status);

			(*status).MPI_SOURCE = -1;
			for(int i=0; i<agg_req->request_count; i++) {
				if(arr_status[i].MPI_SOURCE >= 0 && arr_status[i].MPI_ERROR == MPI_SUCCESS && rank_ignore_list[ arr_status[i].MPI_SOURCE ] != 1) {
					*status = arr_status[i];
					// check if arr_status[0].MPI_SOURCE < 0
					(*status).MPI_SOURCE = rank_2_job[arr_status[i].MPI_SOURCE];
					wa_index = i;
					break;
				}
			}

			if((*status).MPI_SOURCE < 0) {
				gdb_val = 2;
				log_e("NO MPI_SOURCE valid");
				exit(999);
			}
			//PMPI_Abort(MPI_COMM_WORLD, 500);
		}*/
		//************************************************************************

		debug_log_i("Returning: SOURCE: %d", (*status).MPI_SOURCE);
		
		
		/*for(int i=0; i<wa_index; i++) {
			source_buf = source_buf->next;
		}*/

		// Do remaining Irecv.
		int receiver_for_rank;
		if(agg_req->async_type == ANY_RECV) {
			debug_log_i("Doing rest of Irecv calls");
			for(int i=0; i<job_list[(*status).MPI_SOURCE].worker_count; i++) {
				receiver_for_rank = (job_list[ (*status).MPI_SOURCE ].rank_list)[i];
				if(arr_status[wa_index].MPI_SOURCE == receiver_for_rank) {
					continue;
				}
				PMPI_Irecv(__blackhole_address, agg_req->buffer_size, MPI_BYTE, MPI_ANY_SOURCE, SET_TAG(receiver_for_rank, agg_req->tag), node.rep_mpi_comm_world, &blackhole_request);
			}
		}


		// *status = wa_status;
		// (*status).MPI_SOURCE = rank_2_job[wa_status.MPI_SOURCE];

		//log_i("Address: %p | %p", agg_req->original_buffer, source_buf->buf);
		if(agg_req->request_count != 1) {
			void *temp_buf = ((char *)agg_req->temp_buffer) + wa_index * agg_req->buffer_size;
			void *ori_buf = agg_req->original_buffer;
			debug_log_i("Received data: %d | index: %d", temp_buf, wa_index);
			memcpy(ori_buf, temp_buf, agg_req->buffer_size);
		}

		// clean the mess.
		free_agg_request(agg);
		free(arr_status);

		// TODO: Return proper success from MPI_* function.
		debug_log_i("Done with async call");
		return MPI_SUCCESS;
	}
	
}
