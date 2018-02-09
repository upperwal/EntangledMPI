#include <stdio.h>
#include "src/replication/stackseg.h"

int main(int argc, char** argv){
	MPI_Init(NULL, NULL);
	argc = 300;
	//stackMig(&argc);
	MPI_Finalize();
	return 1;
}
