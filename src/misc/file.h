#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "src/shared.h"
#include "src/misc/log.h"

#ifndef __FILE_H__
#define __FILE_H__

int is_file_modified(char *, time_t *, enum CkptBackup *);
void set_last_update(char *, time_t *);

#endif
