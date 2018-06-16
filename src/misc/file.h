#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "src/shared.h"
#include "src/misc/log.h"

#ifndef __FILE_H__
#define __FILE_H__

int is_file_modified(char *, time_t *, enum CkptRestore *);
void set_last_file_update(char *, time_t *);

void save_rep_and_stack_info(int);

#endif
