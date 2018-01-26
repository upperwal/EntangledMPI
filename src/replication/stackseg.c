#include "stackseg.h"


/* 
*
* Stack Migration:
*  	If argc has value (decimal) 01 => (bits) 00000000 00000000 00000000 00000001 => (hex) 00 00 00 01
*  				0000	|________|
*				0001	|________|
*  				0002 UB	|________|
*				0003	|________|
*  				....	|........|
*				1005	|________|
*				1006	|________|	
*				1007	|___01___|	<- &argc (In little endian format)
*				1008	|___00___|
*				1009	|___00___|
*	main ->		100a LB	|___00___|  <- rbp (rbp might not be used after optimisation)
*				100b	|________|
*				100c	|________|
*
* How to migrate stack?
*  	Identify stack address from where main function starts. This will be the upper bound
*  	ie. higher address (as stack grows higher to lower address). Now identify lower bound
*  	this is the address of the most deep function currently in stack.
*
* Idetifying from where main function starts.
*  	Use address of main functions arguments ie. &argc. argc is defined relative to $rbp
*	or $rsp and is the first thing in main function stack frame.
*
* NOTE: Remember to add 4 to &argc to get Lower Bound (LB)
*
*/

int stackMig(int *a) {
	printf("%p\n", a);

	int *arr = {1,2,3,4,5};
	int *arr2;
	int rank;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0) {
		arr2 = arr;
		MPI_Send(arr2, 20, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	}
	else {
		MPI_Recv(arr2, 20, MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
}
