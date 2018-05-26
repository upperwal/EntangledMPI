#include "rep.h"

#include "src/shared.h"

char *newStack;
extern jmp_buf context;
extern enum MapStatus map_status;

/* pthread mutex */
extern pthread_mutex_t global_mutex;
extern pthread_mutex_t rep_time_mutex;
extern pthread_mutex_t comm_use_mutex;

extern address stackLowerAddress;

extern Node node;

extern Malloc_list *head;

extern int __pass_sender_cont_add;
extern int __pass_receiver_cont_add;

extern int __request_pending;

/* 
*  1. Checks for file update pointer from the replication thread.
*  2. If set, do a setjmp(), MPI_wait_all() and switch to alternate stack.
*     sleep untill rep thread send signal.
*/
int is_file_update_set() {
	// This function will execute on main user program thread.
	//printf("Thread: Main | Function: is_file_update_set | newStack Address: %p\n", &newStack);

	if(map_status == MAP_UPDATED && __request_pending == 0) {
		
		log_i("Map Update: %d", map_status);

		int s = setjmp(context);
		if(s == 0) {

			jmp_buf copy_context;

			copy_jmp_buf(context, copy_context);

			// Create some space for new temp stack.
			newStack = malloc(sizeof(char) * 2000);

			debug_log_i("New stack lower address: %p | higher: %p", newStack, newStack + 1800);
			
			// Stack starts from higher address.
			setRSP(copy_context, newStack + 1800);

			// Start execution on new temp stack.
			longjmp(copy_context, 1);
		}
		else if(s == 1) {
			// DO NOT PLACE *log_* macros INSIDE THIS.
			// New stack will start right here.
			//debug_log_i("Back to the past.");

			// Unlock global mutex, so that rep thread can lock it.
			pthread_mutex_unlock(&global_mutex);

			// lock to this mutex will result in suspension of this thread.
			pthread_mutex_lock(&rep_time_mutex);

			//log_i("Main thread block open.");

			// Need to release it.
			pthread_mutex_unlock(&rep_time_mutex);

			// lock to global mutex means user's main thread is ready to execute.
			pthread_mutex_lock(&global_mutex);

			longjmp(context, 2);
		}
		else {
			// Original stack will start here.
			debug_log_i("Works Awesome!!!!");

			// Free space allocated for temp stack.
			free(newStack);

			debug_log_i("After longjmp | newStack address: %p", &newStack);


		}
		return 1;
	}
	else {
		return 0;
	}

}

int init_rep(MPI_Comm job_comm) {
	log_i("Replication Init.");

	// Init Data Segment
	PMPI_Barrier(job_comm);
	transfer_data_seg(job_comm);

	// Init Stack Segment
	PMPI_Barrier(job_comm);
	transfer_stack_seg(job_comm);

	// Init Heap Segment
	PMPI_Barrier(job_comm);
	transfer_heap_seg(job_comm);

	// Send Framework related data
	PMPI_Barrier(job_comm);
	transfer_framework_data(job_comm);

	log_i("Replication Complete.");
}

int transfer_framework_data(MPI_Comm job_comm) {
	PMPI_Bcast(&__pass_sender_cont_add, 1, MPI_INT, 0, job_comm);
	PMPI_Bcast(&__pass_receiver_cont_add, 1, MPI_INT, 0, job_comm);
}

void copy_jmp_buf(jmp_buf source, jmp_buf dest) {
	for(int i = 0; i<8; i++) {
		dest[0].__jmpbuf[i] = source[0].__jmpbuf[i];
	}
	dest[0].__mask_was_saved = source[0].__mask_was_saved;
}

address getPC(jmp_buf context) {
	address rip = context[0].__jmpbuf[JB_PC];
	PTR_DECRYPT(rip);
	return rip;
}

address getRBP(jmp_buf context) {
	address rbp = context[0].__jmpbuf[JB_RBP];
	PTR_DECRYPT(rbp);
	return rbp;
}

address getRSP(jmp_buf context) {
	address rsp = context[0].__jmpbuf[JB_RSP];
	PTR_DECRYPT(rsp);
	return rsp;
}

int setPC(jmp_buf context, address pc) {
	PTR_ENCRYPT(pc);
	context[0].__jmpbuf[JB_PC] = pc;
	return 1;
}

int setRBP(jmp_buf context, address rbp) {
	PTR_ENCRYPT(rbp);
	context[0].__jmpbuf[JB_RBP] = rbp;
	return 1;
}

int setRSP(jmp_buf context, address rsp) {
	PTR_ENCRYPT(rsp);
	context[0].__jmpbuf[JB_RSP] = rsp;
	return 1;
}

unsigned int parseChar(char c) {
	if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + c - 'a';
    if ('A' <= c && c <= 'F') return 10 + c - 'A';

    return 30;	// Error Code: 30
}

address charArray2Long(char *arr) {
	address result = 0;

	result += parseChar(arr[0]);
	for(int i=1; i<strlen(arr) ; i++) {
		result = result << 4;
		result += parseChar(arr[i]);
	}

	return result;
}

int readProcMapFile() {
	FILE *mapFilePtr;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	int lineNumber = 1;

	MPI_Comm comm;

	mapFilePtr = fopen("/proc/self/maps", "r");

	if(mapFilePtr == NULL) {
	    return 1;
	}

	address dataSegStart = NULL, dataSegEnd = NULL;
	while((nread = getline(&line, &len, mapFilePtr)) != -1) {
	    printf("%s", line);

	    if(lineNumber == 2) {
	    	char *secondAddr = NULL;
	    	for(int i=0; i<nread; i++) {
	    		if(line[i] == '-') {
	    			line[i] = '\0';
	    			secondAddr = line + i + 1;
	    		}
	    		else if(line[i] == ' ') {
	    			line[i] = '\0';
	    			break;
	    		}
	    	}

	    	debug_log_i("String 1: %s | String 2: %s", line, secondAddr);
	    	dataSegStart = charArray2Long(line);
	    	dataSegEnd = charArray2Long(secondAddr);
	    }

	    lineNumber++;
	}

	//printf("%d\n", &_edata - &__data_start);


	/*printf("%p | %p\n", dataSegStart, dataSegEnd);
	char *data = dataSegStart;
	for(int i=0; i<300000; i=i+300) {
		*data =
	}
	for(int i=0; i<300000; i=i+300) {
		printf("%p\n", *data);
	}*/

	//extern _etext, _edata, _end;

	return 0;
}
