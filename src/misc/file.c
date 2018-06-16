#include "file.h"

extern Node node;
extern address stackHigherAddress;

typedef struct {
	int replication_idx;
	int rank;
	address stack_address;
} rep_and_stack_info;

int is_file_modified(char *file_name, time_t *last_update, enum CkptRestore *ckpt_restore) {
	struct stat file_attr;

	if(*ckpt_restore == RESTORE_YES) {
		return 1;
	}

	stat(file_name, &file_attr);

	if(file_attr.st_mtime > *last_update) {
		*last_update = file_attr.st_mtime;
		debug_log_i("File Updated.");
		return 1;
	}
	else {
		debug_log_i("File NOT Updated.");
		return 0;
	}
}

void set_last_file_update(char *file_name, time_t *last_update) {
	struct stat file_attr;

	stat(file_name, &file_attr);
	*last_update = file_attr.st_mtime;
}

void save_rep_and_stack_info(int rep_idx) {
	rep_and_stack_info rep_stack;
	MPI_File rep_stack_info_file;
	MPI_Offset offset = sizeof(rep_and_stack_info) * node.rank;

	rep_stack.replication_idx = rep_idx;
	rep_stack.rank = node.rank;
	rep_stack.stack_address = stackHigherAddress;

	// Writting network stats to a file.
	PMPI_File_open(node.rep_mpi_comm_world, STACK_REP_INFO_FILE_NAME, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &rep_stack_info_file);
	PMPI_File_write_at(rep_stack_info_file, offset, &rep_stack, sizeof(rep_and_stack_info), MPI_BYTE, MPI_STATUS_IGNORE);
	PMPI_File_close(&rep_stack_info_file);
}
