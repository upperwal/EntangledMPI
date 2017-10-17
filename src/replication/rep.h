//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <syscall.h>
#include <string.h>
#include <sys/types.h>

#include <dlfcn.h>
#include <stdint.h>

/* MPI PROFILER - BEGIN */

#define REP_DEGREE 1.5 
MPI_Comm MPI_COMM_CMP, MPI_COMM_REP, CMP_REP_INTERCOMM;
int *repToCmpMap = NULL, *cmpToRepMap = NULL;
char *workDir = "/home/mas/16/cdsabhi/project/Code/PaRep-MPI", *repState = "repState";

/* MPI PROFILER - END */

/* REPLICATION MANAGER - BEGIN */

#define CONFIG_FILE cfg
#define CP_FILE ckpt

#define JMP_BUF_SP(tempEnv) (((long*)(tempEnv))[JmpBufSP_Index()])

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


#define PAGE_SIZE sysconf(_SC_PAGESIZE) //getpagesize()
#define PAGE_MASK (~(PAGE_SIZE - 1))

typedef long RAW_ADDR;

RAW_ADDR stackPosInit;
jmp_buf env;
int repRank = -1, nC, nR;

/* MPI_REQ LIST BEGIN */

struct MPIReqStruct
{
  MPI_Request *request;
  MPI_Request *reqRep;
  void *reqRepBuf;
  struct MPIReqStruct *next;
};

struct MPIReqStruct *mpiReqHead = NULL, *mpiReqTail = NULL;

struct MPIReqStruct *findMR (MPI_Request *);
struct MPIReqStruct * createMR (MPI_Request *);
void deleteMR (MPI_Request *);
int countMR (void);

/* MPI_REQ LIST END */

/* HEAP LIST - BEGIN */

struct heapListStruct
{  
  void *ptrAddr;
  void *ptr;
  size_t size;
  struct heapListStruct *next;
};

struct heapListStruct *heapListHead = NULL, *heapListTail = NULL;

int updateHL (void *, void *, size_t);
void appendHL (void *, void *, size_t);
void deleteHL (void *);
int countHL (void);
void displayHL (void);

/* MALLOC/FREE WRAPPER - BEGIN */

void *myMalloc (void *, size_t);
void myFree (void *);

/* MALLOC/FREE WRAPPER - END */

/* HEAP LIST - END */

RAW_ADDR dataSegAddr [2]; 
RAW_ADDR stackSegAddr [2];

int JmpBufSP_Index (void);
unsigned long SPFromJmpBuf (jmp_buf);
int replace (RAW_ADDR bottomAddr, long, int, MPI_Comm);

/* STACK REPLACE - BEGIN */

static void (*savedFunc)();
#define tmpStackSize sizeof(char)*512*1024/sizeof(double)
static double tmpStack [tmpStackSize];
static const long overRunFlag = 0xdeadbeef;
void restoreStack (void);

static long *getOverrunPos (void);
void executeOnTmpStk (void (*)());

/* STACK REPLACE - END */

void replaceImage (int, RAW_ADDR, RAW_ADDR, RAW_ADDR, RAW_ADDR);
int sendData (int);
void sendImage (int, int);
void recvImage (int);
void repManager (void);
