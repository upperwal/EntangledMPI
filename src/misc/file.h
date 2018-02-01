#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef __FILE_H__
#define __FILE_H__

int is_file_modified(char *, time_t *);
void set_last_update(char *, time_t *);

#endif
