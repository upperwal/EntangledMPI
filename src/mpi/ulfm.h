#include <mpi.h>

#ifdef OPEN_MPI
#include <mpi-ext.h>
#endif

#include <assert.h>

#include "src/shared.h"
#include "src/misc/log.h"
#include "src/mpi/comm.h"

#ifndef __ULFM_H__
#define __ULFM_H__

void rep_errhandler(MPI_Comm*, int*, ...);

void update_job_list(int, int *);
int is_failed_node_world_job_comm_root();

#endif
