#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#include "src/shared.h"
#include "src/misc/log.h"

#ifndef __COMM_H__
#define __COMM_H__

int init_node(char *, Job **, Node *);
int parse_map_file(char *, Job **, Node *, enum CkptBackup *);

int create_migration_comm(MPI_Comm *, int *, enum CkptBackup *);

void update_comms();

void print_job_list();

#endif
