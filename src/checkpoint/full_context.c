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

void init_ckpt(char *file_name) {
	printf("Init Ckpt\n");

	char file[100];
	sprintf(file, file_name, node.job_id);
	printf("File to save checkpoint: %s\n", file);
	
	ckpt_file = fopen(file, "wb");

	save_data_seg();
	save_stack_seg();

	fclose(ckpt_file);
}

void save_data_seg() {
	size_t count_bytes_uninit = ((char *)&_edata - (char *)&__data_start);
	size_t count_bytes_init = ((char *)&_end - (char *)&__bss_start);

	printf("Saving data segment: Uninit Size: %d bytes | Init Size: %d bytes\n", count_bytes_uninit, count_bytes_init);

	fwrite(&count_bytes_uninit, sizeof(size_t), 1, ckpt_file);
	fwrite(&__data_start, 1, count_bytes_uninit, ckpt_file);

	fwrite(&count_bytes_init, sizeof(size_t), 1, ckpt_file);
	fwrite(&__bss_start, 1, count_bytes_init, ckpt_file);
}

void save_stack_seg() {
	size_t stack_size;
	Context processContext;

	stackLowerAddress = getRSP(context);
	printf("[UPADTE] RANK: %d | STACK LOWER: %p | RAW: %p\n", node.rank, stackLowerAddress, getRSP(context));

	printf("[Stack Checkpoint] Lower Address: %p | Higher Address: %p\n", stackLowerAddress, stackHigherAddress);

	stack_size = (size_t)get_stack_size();

	processContext.rip = getPC(context);
	processContext.rsp = getRSP(context);
	processContext.rbp = getRBP(context);

	printf("[CHECKPOINT] PC: %p | RSP: %p | RBP: %p\n", processContext.rip, processContext.rsp, processContext.rbp);

	printf("Stack Checkpoint save Size: %d\n", stack_size);

	fwrite(&processContext, sizeof(Context), 1, ckpt_file);
	fwrite(&stack_size, sizeof(size_t), 1, ckpt_file);
	fwrite((void *)stackLowerAddress, 1, stack_size, ckpt_file);
}

void init_ckpt_restore(char *file_name) {
	printf("Init Ckpt Restore\n");

	char file[100];
	sprintf(file, file_name, node.job_id);
	printf("File to restore checkpoint from: %s\n", file);
	
	ckpt_file = fopen(file, "rb");

	restore_data_seg();
	restore_stack_seg();

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

int does_ckpt_file_exists(char *file_name) {
	char file[100];
	sprintf(file, file_name, node.job_id);

	if(access(file, F_OK) != -1) {
		printf("Ckpt file exist: YES | rank: %d\n", node.rank);
		return 1;
	}
	else {
		printf("Ckpt file does not exist: NO | rank: %d\n", node.rank);
		return 0;
	}
}
