#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// sbrk()
#include <mpi.h>
#include <string.h>

#include "src/replication/dataseg.h"
#include "src/replication/stackseg.h"
#include "src/replication/heapseg.h"

#ifndef __REP_H__
#define __REP_H__

int readProcMapFile();
int is_file_update_set();
int check_map_file_changes();

/* 
*  	setjmp/longjmp context related functions. 
*  	#define JB_RBX	0
*	#define JB_RBP	1
*	#define JB_R12	2
*	#define JB_R13	3
*	#define JB_R14	4
*	#define JB_R15	5
*	#define JB_RSP	6
*	#define JB_PC	7
*
*	Ref URL: https://github.com/bminor/glibc/blob/09533208febe923479261a27b7691abef297d604/sysdeps/x86_64/jmpbuf-offsets.h
*/
#define JB_RBP	1
#define JB_RSP	6
#define JB_PC	7

address getPC(jmp_buf);
address getRBP(jmp_buf);
address getRSP(jmp_buf);

int setPC(jmp_buf, address);
int setRBP(jmp_buf, address);
int setRSP(jmp_buf, address);

void copy_jmp_buf(jmp_buf, jmp_buf);

/* 
*  Replication Related Code
*/
int initRep(MPI_Comm);

#endif
