#include <stdint.h>
#include <setjmp.h>

#ifndef __SHARED_H__
#define __SHARED_H__

int sh;
jmp_buf context;

typedef uint64_t address; 

#endif
