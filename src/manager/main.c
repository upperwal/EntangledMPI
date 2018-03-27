#include <stdio.h>
#include "src/replication/funprint.h"
#include "src/replication/rep.h"

int main (void) {
  puts ("Hello World!");
  puts ("This is " PACKAGE_STRING ".");
  print_func(3);

  //int rank;
  //PMPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //if(rank == 1)
  readProcMapFile();
  return 0;
}
