#include "full_context.h"

FILE *ckpt_file;
extern address stackHigherAddress;
extern address stackLowerAddress;

extern Node node;
extern jmp_buf context;

extern __data_start;		// Initialized Data Segment 	: Start
extern _edata;				// Initialized Data Segment		: End

extern __bss_start;			// Uninitialized Data Segment	: Start
extern _end;				// Uninitialized Data Segment	: End

extern int malloc_number_of_allocations;

extern Malloc_list *head;

void init_ckpt(char *file_name) {
	debug_log_i("Init Ckpt");

	char file[100];
	sprintf(file, file_name, node.job_id);
	log_i("File to save checkpoint: %s", file);
	
	ckpt_file = fopen(file, "wb");

	save_data_seg();
	save_stack_seg();
	save_heap_seg();

	fclose(ckpt_file);
}

void save_data_seg() {
	size_t count_bytes_uninit = ((char *)&_edata - (char *)&__data_start);
	size_t count_bytes_init = ((char *)&_end - (char *)&__bss_start);

	debug_log_i("Saving data segment: Uninit Size: %d bytes | Init Size: %d bytes", count_bytes_uninit, count_bytes_init);

	fwrite(&count_bytes_uninit, sizeof(size_t), 1, ckpt_file);
	fwrite(&__data_start, 1, count_bytes_uninit, ckpt_file);

	fwrite(&count_bytes_init, sizeof(size_t), 1, ckpt_file);
	fwrite(&__bss_start, 1, count_bytes_init, ckpt_file);
}

void save_stack_seg() {
	size_t stack_size;
	Context processContext;

	stackLowerAddress = getRSP(context);
	//debug_log_i("[UPADTE] RANK: %d | STACK LOWER: %p | RAW: %p", node.rank, stackLowerAddress, getRSP(context));

	//debug_log_i("[Stack Checkpoint] Lower Address: %p | Higher Address: %p", stackLowerAddress, stackHigherAddress);

	stack_size = (size_t)get_stack_size();

	processContext.rip = getPC(context);
	processContext.rsp = getRSP(context);
	processContext.rbp = getRBP(context);

	//debug_log_i("[CHECKPOINT] PC: %p | RSP: %p | RBP: %p", processContext.rip, processContext.rsp, processContext.rbp);

	debug_log_i("Stack Checkpoint save Size: %d", stack_size);

	fwrite(&processContext, sizeof(Context), 1, ckpt_file);
	fwrite(&stack_size, sizeof(size_t), 1, ckpt_file);
	fwrite((void *)stackLowerAddress, 1, stack_size, ckpt_file);
}

void save_heap_seg() {
	Malloc_list *temp = head;

	// Saving number of memory allocations using malloc.
	fwrite(&malloc_number_of_allocations, sizeof(int), 1, ckpt_file);

	while(temp != NULL) {
		// Saving the memory container.
		fwrite(&(temp->container), sizeof(Malloc_container), 1, ckpt_file);
		debug_log_i("Writing Malloc Container: Address: %p | Size: %d", (temp->container).container_address, (temp->container).size);

		// Saving whatever is in heap memory.
		address *pointer_to_heap = (address *)(temp->container).container_address;
		address *tmp = *pointer_to_heap;
		debug_log_i("Saving stack bytes: %d", *tmp);

		// Handling the case if *pointer_to_heap == NULL
		fwrite((void *)(*pointer_to_heap), (temp->container).size, 1, ckpt_file);
		temp = temp->next;
	}
}

void init_ckpt_restore(char *file_name) {
	log_i("Init Ckpt Restore");

	char file[100];
	sprintf(file, file_name, node.job_id);
	log_i("File to restore checkpoint from: %s", file);
	
	ckpt_file = fopen(file, "rb");

	restore_data_seg();
	restore_stack_seg();
	restore_heap_seg();

	fclose(ckpt_file);
}

void restore_data_seg() {
	size_t count_bytes_uninit;
	size_t count_bytes_init;

	fread(&count_bytes_uninit, sizeof(size_t), 1, ckpt_file);
	fread(&__data_start, 1, count_bytes_uninit, ckpt_file);

	fread(&count_bytes_init, sizeof(size_t), 1, ckpt_file);
	fread(&__bss_start, 1, count_bytes_init, ckpt_file);
}

void restore_stack_seg() {
	size_t stack_size;
	Context processContext;

	fread(&processContext, sizeof(Context), 1, ckpt_file);
	fread(&stack_size, sizeof(size_t), 1, ckpt_file);

	setPC(context, processContext.rip);
	setRSP(context, processContext.rsp);
	setRBP(context, processContext.rbp);

	stackLowerAddress = stackHigherAddress - (stack_size - 4); 

	fread((void *)stackLowerAddress, 1, stack_size, ckpt_file);
}

void restore_heap_seg() {
	// Restore heap allocation number
	fread(&malloc_number_of_allocations, sizeof(int), 1, ckpt_file);

	Malloc_container temp_container;

	for(int i=0; i<malloc_number_of_allocations; i++) {
		fread(&temp_container, sizeof(Malloc_container), 1, ckpt_file);

		address *pointer_to_heap = (address *)temp_container.container_address;

		*pointer_to_heap = malloc(temp_container.size);

		fread(*pointer_to_heap, temp_container.size, 1, ckpt_file);

		rep_append(temp_container);
	}
}

int does_ckpt_file_exists(char *file_name) {
	char file[100];
	sprintf(file, file_name, node.job_id);

	if(access(file, F_OK) != -1) {
		log_i("Ckpt file exist: YES");
		return 1;
	}
	else {
		log_i("Ckpt file does not exist: NO");
		return 0;
	}
}
