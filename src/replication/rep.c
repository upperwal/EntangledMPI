#include "src/shared.h"
#include "rep.h"

int check_for_map_update() {
	printf("Map Update: %d\n", sh);
	if(setjmp(context) == 0) {
		printf("Hello JMP\n");
		longjmp(context, 1);
	}
	else {
		printf("Back to the past.\n");
	}
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

	    	printf("String 1: %s | String 2: %s\n", line, secondAddr);
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
