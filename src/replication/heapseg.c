#include "heapseg.h"

Malloc_list *head = NULL;
Malloc_list *tail = NULL;

size_t total_malloc_allocation_size = 0;
int malloc_number_of_allocations = 0;

// Memory allocated through rep_malloc will be discontiguous.
// Memory allocated during replica receive will be continous.
Memory_state mem_state = DISCONTIGUOUS;

void rep_append(Malloc_container container) {
	Malloc_list *element = malloc(sizeof(Malloc_list));

	element->next = NULL;
	element->prev = tail;
	
	(element->container).container_address = container.container_address;
	(element->container).linked_address = container.linked_address;
	(element->container).allocated_address = container.allocated_address;
	(element->container).size = container.size;

	if(head == NULL) {
		head = element;
	}
	else {
		tail->next = element;
	}
	tail = element;
}

void rep_remove(void *node_add) {
	Malloc_list *element = (Malloc_list *)node_add;
	Malloc_list *prev = element->prev;

	if(element -> next == NULL && element -> prev == NULL) {
		free(element);
		head = NULL;
		tail = NULL;
		return;
	}

	if(element -> next != NULL) {
		(element->next)->prev = prev;
	}

	if(prev != NULL) {
		prev->next = element->next;
	}	

	free(element);
}

void rep_display() {
	Malloc_list *temp = head;
	while(temp != NULL) {
		log_i("[Display] Container Address: %p | Size: %d", (temp->container).container_address, (temp->container).size);
		temp = temp->next;
	}
}

// NOT yet used. Not Tested.
void rep_clear() {
	
	address *ptr;
	ptr = (head->container).allocated_address;
	
	free(ptr);
	Malloc_list *temp = head;
	while(temp != NULL) {
		head = temp;
		temp = temp->next;
		free(head);
	}
	head = NULL;
	tail = NULL;
	malloc_number_of_allocations = 0;
	
}

void rep_clear_discontiguous() {
	debug_log_i("Clearing Malloc List | no mallocs: %d", malloc_number_of_allocations);
	Malloc_list *temp = head;
	address *ptr;
	

	while(temp != NULL) {

		head = temp;
		temp = temp->next;

		ptr = (head->container).allocated_address;
		debug_log_i("First Ptr");
		free(ptr);
		debug_log_i("Second Ptr");
		free(head);
	}
	head = NULL;
	tail = NULL;
	malloc_number_of_allocations = 0;
	debug_log_i("Clearing Malloc List End");
}

void assign_malloc_context(const void **source, const void **dest) {
	Malloc_container cont;
	cont.container_address = (address)dest;
	cont.linked_address = (address)source;
	cont.size = 0;

	cont.allocated_address = *source;

	malloc_number_of_allocations++;

	rep_append(cont);

	*dest = *source;
}

void rep_malloc(void **container, size_t size) {
	//address **container = (address **)cond_add;
	*container = malloc(size);

	Malloc_container cont;
	cont.container_address = (address)container;
	cont.linked_address = 0;
	cont.allocated_address = (address)*container;
	cont.size = size;

	

	total_malloc_allocation_size += size;
	malloc_number_of_allocations++;

	rep_append(cont);
	debug_log_i("[Malloc] Head :- Add: %p | Value: %p", &head, head);
	debug_log_i("[Malloc] Container :- Add: %p | LAdd: %p | Size: %d", (head->container).container_address, (head->container).linked_address, (head->container).size);
	//address *l = cont.container_address;

	//printf("Container Address: %p | Size: %d | malloc Address: %p\n", (head->container).container_address, (head->container).size, *l);
}

void rep_free(void **cont_add) {
	Malloc_list *temp = head;

	// Empty linkedlist
	while(temp != NULL) {
		if((temp->container).container_address == cont_add) {
			free((temp->container).allocated_address);
			rep_remove(temp);
			break;
		}
		//printf("[Display] Container Address: %p | Size: %d\n", (temp->container).container_address, (temp->container).size);
		temp = temp->next;
	}
}

int transfer_heap_seg(MPI_Comm job_comm) {
	log_i("Heap Segment Init");
	
	//Memory_state prev_mem_state = mem_state;
	address heap_start;
	Malloc_container container;
	Malloc_list *temp = head;
	int rank;
	PMPI_Comm_rank(job_comm, &rank);

	//printf("Rank: %d | Yes.\n", rank);

	PMPI_Bcast((void *)&total_malloc_allocation_size, 1, MPI_INT, 0, job_comm);
	debug_log_i("total_malloc_allocation_size: %d", total_malloc_allocation_size);
	

	// TODO: Replace rank with NODE_DATA_RECEIVER
	if(rank != 0) {
		// free already allocated memory.
		// This wont be used much. As data will become non-contiguous as new malloc is done
		// on replica node.
		debug_log_i("Clearing Old Heap for replica.");
		if(mem_state == CONTIGUOUS) {
			rep_clear();
		}
		else {
			rep_clear_discontiguous();
		}

		//heap_start = (address)malloc(total_malloc_allocation_size);
	}

	debug_log_i("Is Head NULL: %d", head == NULL);

	rep_display();


	PMPI_Bcast((void *)&malloc_number_of_allocations, 1, MPI_INT, 0, job_comm);
	debug_log_i("malloc_number_of_allocations: %d", malloc_number_of_allocations);
	//PMPI_Bcast((void *)&mem_state, 1, MPI_INT, 0, job_comm);


	
	//printf("Rank: %d | SPECIAL. | malloc_number_of_allocations: %d\n", rank, malloc_number_of_allocations);
	if(head != NULL)
		debug_log_i("Container value Head: %p", (head->container).container_address);

	//printf("[After heap alloc] Rank: %d | malloc_number_of_allocations: %d\n", rank, malloc_number_of_allocations);
	for(int i = 0; i < malloc_number_of_allocations; i++) {
		debug_log_i("Malloc Number: %d", i);
		if(rank == 0) {
			container.container_address = (temp->container).container_address;
			container.linked_address = (temp->container).linked_address;
			container.allocated_address = (temp->container).allocated_address;
			container.size = (temp->container).size;

			temp = temp->next;
		}

		

		//printf("FOR: 1 Rank: %d\n", rank);

		PMPI_Bcast((void *)&container, sizeof(Malloc_container), MPI_BYTE, 0, job_comm);
		debug_log_i("Container Info: Add: %p | LAdd: %p | Size: %d", container.container_address, container.linked_address,container.size);
		//printf("FOR: 2 Rank: %d\n", rank);

		debug_log_i("After Container BCast");

		if(rank != 0) {
			address remalloc_address = realloc(container.allocated_address, container.size);
			debug_log_i("Remalloc Address: %p", remalloc_address);
		}

		debug_log_i("Container allocated Address: %p", container.allocated_address);

		PMPI_Bcast((void *)container.allocated_address, container.size, MPI_BYTE, 0, job_comm);

		/*if(container.size == 0 && container.linked_address != 0) {
			//printf("POINTER PROBLEM: | linked_address: %p | container_address: %p\n", container.linked_address, container.container_address);
			if(rank != 0) {
				address *src, *dest;
				src = container.linked_address;
				dest = container.container_address;

				*dest = *src;
			}
			continue;
		}

		if(rank == 0) {
			address *ptr = container.container_address;
			heap_start = *ptr;
		}
		
		PMPI_Bcast((void *)heap_start, container.size, MPI_BYTE, 0, job_comm);

		address *ii = (address *)heap_start;
		debug_log_i("Heap Sent Data: %d | Container Address: %p", *ii, container.container_address);

		if(rank != 0) {
			address *ptr = container.container_address;

			*ptr = heap_start;
			debug_log_i("Added new Heap Mem: %p", heap_start);

			heap_start += container.size;

			rep_append(container);
		}*/
	}

	log_i("Heap Seg Transfer Ended.");
}
