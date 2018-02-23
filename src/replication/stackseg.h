#include <stdio.h>
#include <mpi.h>

#include "src/misc/log.h"

#ifndef __STACK_SEG_H__
#define __STACK_SEG_H__

int transfer_stack_seg(MPI_Comm);
int get_stack_size();

#endif
