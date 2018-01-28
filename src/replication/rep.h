#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// sbrk()
#include <mpi.h>
#include <string.h>

#ifndef __REP_H__
#define __REP_H__

int readProcMapFile();
int check_for_map_update();

#endif
