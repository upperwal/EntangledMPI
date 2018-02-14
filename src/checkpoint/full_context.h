#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "src/shared.h"
#include "src/replication/rep.h"

#ifndef __FULL_CONTEXT_H__
#define __FULL_CONTEXT_H__

void init_ckpt(char *);
void save_data_seg();
void save_stack_seg();

void init_ckpt_restore(char *);
void restore_data_seg();
void restore_stack_seg();

int does_ckpt_file_exists(char *);

#endif
