#include <stdint.h>
#include <setjmp.h>
#include <pthread.h>

#ifndef __SHARED_H__
#define __SHARED_H__

// Map is a file or API response which maps compute node to a replica node. If the
// hash of the file/API is changed MAP_UPDATED status is used.
enum MapStatus { MAP_NOT_UPDATED, MAP_UPDATED };
typedef uint64_t address; 

jmp_buf context;
enum MapStatus map_status;

#define PTR_ENCRYPT(variable) \
        asm ("xor %%fs:0x30, %0\n\t" \
        "rol $17, %0" \
        : "=r" (variable) \
        : "0" (variable)); 

#define PTR_DECRYPT(variable) \
        asm ("ror $17, %0\n\t" \
        "xor %%fs:0x30, %0" \
        : "=r" (variable) \
        : "0" (variable));


// pthread mutex
pthread_mutex_t global_mutex;
pthread_mutex_t rep_time_mutex;

#endif
