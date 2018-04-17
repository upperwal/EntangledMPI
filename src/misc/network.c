#include "network.h"

#define HOSTNAME_LEN 100

extern Node node;

typedef struct {
	int rank;
	pid_t process_id;
	int hostname_len;
	char hostname[HOSTNAME_LEN];
} network_stat;

void network_stat_init(char *network_stat_filename) {
	network_stat net_stat;
	MPI_File network_stat_file;
	MPI_Offset offset = sizeof(network_stat) * node.rank;

	gethostname(net_stat.hostname, HOSTNAME_LEN);
	net_stat.rank = node.rank;
	net_stat.hostname_len = strlen(net_stat.hostname);
	net_stat.process_id = getpid();

	debug_log_i("Rank: %d | PID: %d | Hostname: %s", net_stat.rank, net_stat.process_id, net_stat.hostname);

	// Writting network stats to a file.
	PMPI_File_open(node.rep_mpi_comm_world, network_stat_filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &network_stat_file);
	PMPI_File_write_at(network_stat_file, offset, &net_stat, sizeof(network_stat), MPI_BYTE, MPI_STATUS_IGNORE);
	PMPI_File_close(&network_stat_file);
}
