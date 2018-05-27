#include "async.h"

extern int *rank_2_job;

int r_count;
MPI_Request *arr_request;
MPI_Status *arr_status;
MPI_Status wa_status;

Aggregate_Request *new_agg_request(void *ori_buf, MPI_Datatype datatype, int count) {
	Aggregate_Request *agg_req = (Aggregate_Request *)malloc(sizeof(Aggregate_Request));

	int type_size;
	PMPI_Type_size(datatype, &type_size);

	agg_req->list = NULL;
	agg_req->buf_list = NULL;
	agg_req->request_count = 0;
	agg_req->original_buffer = ori_buf;
	agg_req->buffer_size = type_size * count;

	return agg_req;
}

MPI_Request *add_new_request_and_buffer(Aggregate_Request *agg, void **buf_add) {
	MPI_Request *request = (MPI_Request *)malloc(sizeof(MPI_Request));

	Request_List *req_list = (Request_List *)malloc(sizeof(Request_List));
	req_list->req = request;
	req_list->next = agg->list;

	
	Buffer_List *buf_list = (Buffer_List *)malloc(sizeof(Buffer_List));
	buf_list->buf = (void *)malloc(agg->buffer_size);
	buf_list->next = agg->buf_list;
	*buf_add = buf_list->buf;

	agg->list = req_list;
	agg->buf_list = buf_list;
	agg->request_count++;

	debug_log_i("Request Address Allocation: %p", request);

	return request;
}

int get_request_count(void *agg) {
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	return agg_req->request_count;
}

MPI_Request *get_array_of_request(void *agg, int *size) {
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

		/*MPI_Status sta;

		debug_log_i("Before LIST REQ: %d", *size);
		PMPI_Wait(array + i, &sta);
		debug_log_i("After LIST REQ");*/
	}

	return array;
}

int free_agg_request(void *agg) {
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	Request_List *list = agg_req->list;
	Buffer_List *buf_list = agg_req->buf_list;
	for(int i=1; i<agg_req->request_count; i++) {

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
	}

	free(agg_req);

	return MPI_SUCCESS;
}

int wait_for_agg_request(void *agg, MPI_Status *status) {
	
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	arr_request = get_array_of_request(agg, &r_count);

	if(r_count == 0 || arr_request == NULL) {
		log_e("arr_request is NULL");
		return MPI_SUCCESS;
	}

	int wa_index;
	int result;

	if(status == MPI_STATUS_IGNORE) {
		while(PMPI_Waitany(r_count, arr_request, &wa_index, status));
		//while(PMPI_Waitall(r_count, arr_request, MPI_STATUSES_IGNORE) != MPI_SUCCESS);

		free_agg_request(agg);
		free(arr_request);

		return MPI_SUCCESS;
	}
	else {
		//arr_status = (MPI_Status *)malloc(sizeof(MPI_Status) * r_count);

		//PMPI_Wait(arr_request, arr_status);

		// MPI_Waitall is giving some wierd seg fault. 
		//while(PMPI_Waitall(r_count, arr_request, arr_status) != MPI_SUCCESS);
		
		

		// because we are ignoring process failure in send/recv
		// if a process failure occur below function will go crazy. Err handler will
		// be invoked again and again.
		// Could be optimized by correcting the job map and sending data to only alive nodes.
		// Other possibility would be to create an ignore map. Which include all failed nodes.
		// Added an ignore map.
		int wstatus = PMPI_Waitany(r_count, arr_request, &wa_index, &wa_status);

		// TODO: while loop around PMPI_Waitany
		if(wstatus != MPI_SUCCESS) {
			log_i("waitany failed");

			wstatus = PMPI_Waitany(r_count, arr_request, &wa_index, &wa_status);
			if(wstatus != MPI_SUCCESS)
				PMPI_Abort(MPI_COMM_WORLD, 400);

			log_i("Done wait any");
		}

		// TODO: Need to update the MPI_SOURCE 
		// Current implementation will send the original rank and not the job no.
		/*for(int i=0; i<r_count; i++) {
			if(arr_status[i].MPI_ERROR != MPI_SUCCESS) {
				(*status).MPI_SOURCE = rank_2_job[arr_status[i].MPI_SOURCE];
				(*status).MPI_TAG = arr_status[i].MPI_TAG;
				(*status).MPI_ERROR = arr_status[i].MPI_ERROR;

				return MPI_SUCCESS;
			}
		}*/

		// TODO: IMPORTANT
		// What if nodes associated with arr_status[0] dies?
		// What happens then?

		/**status = arr_status[0];
		(*status).MPI_SOURCE = rank_2_job[arr_status[0].MPI_SOURCE];*/

		Buffer_List *source_buf = agg_req->buf_list;
		for(int i=0; i<wa_index; i++) {
			source_buf = source_buf->next;
		}

		*status = wa_status;
		(*status).MPI_SOURCE = rank_2_job[wa_status.MPI_SOURCE];

		//log_i("Address: %p | %p", agg_req->original_buffer, source_buf->buf);
		void *ori_buf = agg_req->original_buffer;
		memcpy(agg_req->original_buffer, source_buf->buf, agg_req->buffer_size);

		// clean the mess.
		//free_agg_request(agg);
		//free(arr_request);

		// TODO: Return proper success from MPI_* function.
		return MPI_SUCCESS;
	}
	
}
