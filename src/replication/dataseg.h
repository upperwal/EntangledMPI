#include <stdio.h>
#include <mpi.h>

#include "src/mpi/comm.h"

#ifndef __DATA_SEG_H__
#define __DATA_SEG_H__

int transferDataSeg(MPI_Comm);
int transferInitDataSeg(MPI_Comm);
int transferUnInitDataSeg(MPI_Comm);

#endif
