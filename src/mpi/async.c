#include "async.h"

extern int *rank_2_job;

Aggregate_Request *new_agg_request() {
	Aggregate_Request *agg_req = (Aggregate_Request *)malloc(sizeof(Aggregate_Request));
	agg_req->list = NULL;
	agg_req->request_count = 0;

	return agg_req;
}

MPI_Request *add_new_request(Aggregate_Request *agg) {
	MPI_Request *request = (MPI_Request *)malloc(sizeof(MPI_Request));

	Request_List *req_list = (Request_List *)malloc(sizeof(Request_List));
	req_list->req = request;
	req_list->next = agg->list;

	agg->list = req_list;
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
	for(int i=0; i<agg_req->request_count; i++) {

		free(list->req);
		free(list);

		if(list->next != NULL)
			list = list->next;
	}

	free(agg_req);

	return MPI_SUCCESS;
}

int wait_for_agg_request(void *agg, MPI_Status *status) {
	int r_count;
	MPI_Request *arr_request;
	MPI_Status *arr_status;
	Aggregate_Request *agg_req = (Aggregate_Request *)agg;

	arr_request = get_array_of_request(agg, &r_count);

	if(r_count == 0) {
		return MPI_SUCCESS;
	}

	int wa_index;
	int result;

	if(status == MPI_STATUS_IGNORE) {
		//result = PMPI_Waitany(r_count, arr_request, &wa_index, status);
		result = PMPI_Waitall(r_count, arr_request, MPI_STATUSES_IGNORE);

		free_agg_request(agg);
		free(arr_request);

		return result;
	}
	else {
		arr_status = (MPI_Status *)malloc(sizeof(MPI_Status) * r_count);

		//PMPI_Wait(arr_request, arr_status);
		PMPI_Waitall(r_count, arr_request, arr_status);
		//MPI_Status wa_status;
		//result = PMPI_Waitany(r_count, arr_request, &wa_index, &wa_status);

		// TODO: Need to update the MPI_SOURCE 
		// Current implementation will send the original rank and not the job no.
		for(int i=0; i<r_count; i++) {
			if(arr_status[i].MPI_ERROR != MPI_SUCCESS) {
				(*status).MPI_SOURCE = rank_2_job[arr_status[i].MPI_SOURCE];
				(*status).MPI_TAG = arr_status[i].MPI_TAG;
				(*status).MPI_ERROR = arr_status[i].MPI_ERROR;

				return MPI_SUCCESS;
			}
		}

		*status = arr_status[0];
		(*status).MPI_SOURCE = rank_2_job[arr_status[0].MPI_SOURCE];

		//*status = wa_status;
		//(*status).MPI_SOURCE = rank_2_job[wa_status.MPI_SOURCE];

		free_agg_request(agg);
		free(arr_request);

		return result;
	}
	
}
