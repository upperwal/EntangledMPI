#include "file.h"

int is_file_modified(char *file_name, time_t *last_update) {
	struct stat file_attr;

	stat(file_name, &file_attr);
	printf("Time: %d\n", file_attr.st_mtime);

	if(file_attr.st_mtime > *last_update) {
		*last_update = file_attr.st_mtime;
		printf("FILE: Updated.\n");
		return 1;
	}
	else {
		printf("FILE: No Update.\n");
		return 0;
	}
}

void set_last_update(char *file_name, time_t *last_update) {
	struct stat file_attr;

	stat(file_name, &file_attr);
	*last_update = file_attr.st_mtime;
}
