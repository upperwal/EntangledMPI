#include <stdio.h>
#include "src/replication/rep.h"

int main(int argc, char** argv){
int rank, size, len;
char procName[100];
MPI_Comm comm = MPI_COMM_WORLD;
  printf("Hello\n");
  MPI_Init(&argc, &argv);
  
  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  printf("Hello after size and rank functions\n");

  MPI_Barrier(comm);

  //int *temp = (int *) malloc(sizeof(int)*20);

  MPI_Get_processor_name(procName,&len);

  printf("Hello from Process %d of %d on %s\n", rank, size, procName);


  MPI_Finalize();
}
