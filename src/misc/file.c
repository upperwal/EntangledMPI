#include "file.h"

int is_file_modified(char *file_name, time_t *last_update, enum CkptBackup *ckpt_backup) {
	struct stat file_attr;

	if(*ckpt_backup == BACKUP_YES) {
		return 1;
	}

	stat(file_name, &file_attr);

	if(file_attr.st_mtime > *last_update) {
		*last_update = file_attr.st_mtime;
		debug_log_i("File Updated.");
		return 1;
	}
	else {
		debug_log_i("File NOT Updated.");
		return 0;
	}
}

void set_last_update(char *file_name, time_t *last_update) {
	struct stat file_attr;

	stat(file_name, &file_attr);
	*last_update = file_attr.st_mtime;
}
