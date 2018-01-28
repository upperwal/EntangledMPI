#ifndef __REP_H__
#define __REP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>		// sbrk()
#include <mpi.h>
#include <string.h>

typedef uint64_t address; 

int readProcMapFile();

#endif
