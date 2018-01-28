#include <stdio.h>
#include <mpi.h>

#ifndef __DATA_SEG_H__
#define __DATA_SEG_H__

int transferDataSeg(int, MPI_Comm);
int transferInitDataSeg(int, MPI_Comm);
int transferUnInitDataSeg(int, MPI_Comm);

#endif
