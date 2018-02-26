#include <stdio.h>
#include <mpi.h>

#include "src/mpi/comm.h"
#include "src/misc/log.h"

#ifndef __DATA_SEG_H__
#define __DATA_SEG_H__

int transfer_data_seg(MPI_Comm);
int transfer_init_data_seg(MPI_Comm);
int transfer_uninit_data_seg(MPI_Comm);

#endif
