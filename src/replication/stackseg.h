#include <stdio.h>
#include <mpi.h>

#ifndef __STACK_SEG_H__
#define __STACK_SEG_H__

int transfer_stack_seg(MPI_Comm);
int get_stack_size();

#endif
