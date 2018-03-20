#include "ulfm.h"

extern Node node;

int ulfm_detect(int status) {
	int err_class;
	PMPI_Error_class(status, &err_class);

	debug_log_i("Detecting process failure: Status: %d | Error class: %d | MPIX_ERR_PROC_FAILED: %d", status, err_class, MPIX_ERR_PROC_FAILED);
	if(err_class == MPIX_ERR_PROC_FAILED) {
		ulfm_recover();
	}
}

int ulfm_recover() {
	debug_log_i("Init recovery from process failure");
	MPI_Comm new_world_job_comm;

	PMPIX_Comm_shrink(node.world_job_comm, &new_world_job_comm);

	debug_log_i("after shrink");

	int rank, size;
	PMPI_Comm_rank(new_world_job_comm, &rank);
	PMPI_Comm_size(new_world_job_comm, &size);

	debug_log_i("[ULFM] Rank: %d | Size: %d", rank, size);
}
