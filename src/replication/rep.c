#include "rep.h"

/* FUNCTIONS TO KEEP TRACK OF MPI_REQUESTs - BEGIN */

/* In the current implementation, process image replacement shouldn't happen when there are in-flight messages in the system. Pending MPI_Request's indicate the possibility of the presence of in-flight messages. Below set of functions are used to keep track of MPI_Request's using a linked list of the data structure 'struct MPIReqStruct' for the purpose of making sure that there are no requests pending before doing a process replace */

/* Function to create an object of MPIReqStruct using a given request handle and add it to the linked list */
struct MPIReqStruct *createMR (MPI_Request *request)
{
  struct MPIReqStruct *temp = (struct MPIReqStruct *) myMalloc (&temp, sizeof (struct MPIReqStruct));
  temp->request = request;
  temp->reqRep = NULL;
  temp->reqRepBuf = NULL;
  temp->next = NULL;

  if (mpiReqHead == NULL && mpiReqTail == NULL)
  {
    mpiReqHead = temp;
    mpiReqTail = temp;
  }

  else
  {
    mpiReqTail->next = temp;
    mpiReqTail = temp;
  }

  return temp;
}

/* Function to find the corresponding object of type 'struct MPIReqStruct' for a given request handle if present in the linked list for tracking MPI_Request's */
struct MPIReqStruct *findMR (MPI_Request *request)
{
  struct MPIReqStruct *temp = mpiReqHead;

  while (temp != NULL)
  {
    if (temp->request == request)
      return temp;

    else
      temp = temp->next;
  }

  return NULL;
}

/* Function to delete the object corresponding to the given request handle from the linked list for tracking MPI_Request's */
void deleteMR (MPI_Request *request)
{
  struct MPIReqStruct *temp = mpiReqHead;

  if (temp->request == request)
  {
    mpiReqHead == temp->next;
    myFree (temp);
    return;
  }

  while (temp->next != NULL)
  {
     if (temp->next->request == request)
     {
       struct MPIReqStruct *toBeDeleted = temp->next;
       temp->next = temp->next->next;

       if (mpiReqTail == toBeDeleted)
         mpiReqTail = temp;

       myFree (toBeDeleted);
       break;
     }

     temp = temp->next;
  }
}

/* Function to count the number of pending MPI_Request's currently in the system */
int countMR ()
{
  int count = 0;
  struct MPIReqStruct *temp = mpiReqHead;

  while (temp != NULL)
  {
    count ++;
    temp = temp->next;
  }

  return count;
}

/* FUNCTIONS TO KEEP TRACK OF MPI_REQUESTs - END */

/* FUNCTIONS TO KEEP TRACK OF HEAP DATA STRCTURES - BEGIN */

/* Below set of functions are used to keep track of heap data structures in the sytem using a linked list of objects of a data structure 'struct heapListStruct' (hereafter referred to as 'heap list') */

/* Function to modify an entry in the heap list */
int updateHL (void *ptr, void *ptrAddr, size_t size)
{
  struct heapListStruct *temp = heapListHead;

  while (temp != NULL)
  {
    if (temp->ptrAddr == ptrAddr)
    {
      temp->ptr = ptr;
      temp->size = size;
      return 0;
    }

    else
      temp = temp->next;
  }

  return 1;
}

/* Function to add a new entry to the heap list when a new heap allocation is made */
void appendHL (void *ptr, void *ptrAddr, size_t size)
{ 
  //void *(*libc_malloc) (size_t) = dlsym (RTLD_NEXT, "malloc");
  
  //struct heapListStruct *temp = (struct heapListStruct *) libc_malloc (sizeof (struct heapListStruct));
  struct heapListStruct *temp = (struct heapListStruct *) malloc (sizeof (struct heapListStruct));

  temp->ptrAddr = ptrAddr;
  temp->ptr = ptr;
  temp->size = size;
  temp->next = NULL;
  
  if (heapListHead == NULL && heapListTail == NULL)
  {
    heapListHead = temp;
    heapListTail = temp;
  }

  else
  {
    heapListTail->next = temp;
    heapListTail = temp;
  }
}

/* Function to delete an entry from the heap list */
void deleteHL (void *ptr)
{ 
  //void (*libc_free) (void*) = dlsym (RTLD_NEXT, "free");

  struct heapListStruct *temp = heapListHead;

  if (temp->ptr == ptr)
  {
    heapListHead == temp->next;

    //libc_free (temp);
    free (temp);
    
    return;
  }

  while (temp->next != NULL)
  {
     if (temp->next->ptr == ptr)
     {
       struct heapListStruct *toBeDeleted = temp->next;
       temp->next = temp->next->next;
   
       if (heapListTail == toBeDeleted)
         heapListTail = temp;

       //libc_free (toBeDeleted);
       free (toBeDeleted);

       break;
     }

     temp = temp->next;
  }
}

/* Function to count the number of elements in the heap list */
int countHL ()
{
  int count = 0;
  struct heapListStruct *temp = heapListHead;

  while (temp != NULL)
  {
    count ++;
    temp = temp->next;
  }

  return count;
}

/* Function to display the contents of the heap list (for debugging purposes) */
void displayHL ()
{
  struct heapListStruct *temp = heapListHead;

  while (temp != NULL)
  {
    printf ("HL %p\t%p\t%ld\n", temp->ptrAddr, temp->ptr, temp->size);
    temp = temp->next;
  }
}

/* Wrapper function for malloc function, to keep track of heap allocations using heap list */
void *myMalloc (void *pAddr, size_t size)
{
  //void *(*libc_malloc) (size_t) = dlsym (RTLD_NEXT, "malloc");
  //void *p = libc_malloc (size);

  void *p = (void *) malloc (size);

  if (updateHL (p, pAddr, size) != 0)
    appendHL (p, pAddr, size);

  return p;
}

/* Wrapper function for free function, to keep track of heap allocations using heap list */
void myFree (void *p)
{
  //void (*libc_free) (void *) = dlsym (RTLD_NEXT, "free");

  deleteHL (p);

  //libc_free (p);
  free (p);
}

/* FUNCTIONS TO KEEP TRACK OF HEAP DATA STRCTURES - END */

/* Function that returns the index of Stack Pointer in the jmp_buf data structure.
IMPORTANT: This index is system dependent */

int JmpBufSP_Index()
{
  //#if defined(__x86_64__)
    return 6;
  //#endif
}

/* Function to extract the address of the Stack Pointer from jmp_buf */
unsigned long SPFromJmpBuf (jmp_buf tempEnv)
{
  unsigned long addr;
  addr = (long) JMP_BUF_SP(tempEnv);

  /* Whether the below decrypt step (PTR_DECRYPT) is required depends on the implementation of jmp_buf, i.e whether the stack pointer is encrypted before storing it in jmp_buf or not. It may or may not be required. Also, the decrypt implemation is system dependent (refer to mpiProfiler.h for a sample implementation for this) */
  PTR_DECRYPT(addr);

  return addr;
}

/* Function to replace part of the memory (size 'len'), staring from bottomAddr */

int replace (RAW_ADDR bottomAddr, long len, int tag, MPI_Comm comm)
{
  int rank = repRank, retVal;
  MPI_Status status;
  
  printf("Recv inside replace\n"); 
  retVal = PMPI_Recv ((void *) bottomAddr, len, MPI_BYTE, rank, tag, comm, &status);
  printf("Data Received\n");
  repRank = rank;

  if (retVal == MPI_SUCCESS)
    return 0;

  else
    return -1;
}

/* Function to replace the stack segment */
void restoreStack ()
{
     printf("Inside Restaore Stack\n");

  // Author: Abhishek
     jmp_buf thisProcessEnv;
     setjmp(thisProcessEnv);
     int stackSize = stackSegAddr [1] - stackSegAddr [0] + 1;
     stackSegAddr [1] = thisProcessEnv[0].__jmpbuf[1];
     PTR_DECRYPT(stackSegAddr[1]);
     //stackSegAddr [1] = (stackPosInit & PAGE_MASK) + PAGE_SIZE - 1;
     stackSegAddr [0] = stackSegAddr [1] - stackSize + 1;

     if (replace (stackSegAddr [0], stackSize, 61, CMP_REP_INTERCOMM) < 0)
     {
     	printf ("Replacing STACK Segment Unsuccessful!\n");
        PMPI_Abort (MPI_COMM_WORLD, 1);
     }
    
     printf("After replace called in restreStack\n");
  
     PTR_ENCRYPT(stackSegAddr [1]);
     PTR_ENCRYPT(stackSegAddr [0]);

     printf("Both pointers encrypted\n");
  
     env[0].__jmpbuf[6] = stackSegAddr [0];
     env[0].__jmpbuf[1] = stackSegAddr [1];
     
     RAW_ADDR rip = env[0].__jmpbuf[7];
     PTR_ENCRYPT(rip); // Because each machone has a different encrypting key
     env[0].__jmpbuf[7] = rip;

     printf("env set\n");

     longjmp (env, 1);
}

/* Return the address of the temporary stack to be used while replacing the real stack segment */
static long *getOverrunPos()
{
  return (long*) tmpStack;
}

/* Function to move the execution to the temporary stack before replacing the real stack segment */
void executeOnTmpStk (void (*func)())
{
  printf("inside exec\n");
  jmp_buf tempEnv;
  savedFunc = func;
  unsigned long addr;

  *(getOverrunPos()) = overRunFlag;

  if (setjmp (tempEnv) == 0 ) 
  {
    printf("setjmp first time for stack\n");
    addr = (long)(tmpStack + tmpStackSize - 2);

  /* Whether the below encrypt step (PTR_ENCRYPT) is required depends on the implementation of jmp_buf, i.e whether the stack pointer is encrypted before storing it in jmp_buf or not. It may or may not be required. Also, the encrypt implemation is system dependent (refer to mpiProfiler.h for a sample implementation for this) */
    PTR_ENCRYPT(addr);

    JMP_BUF_SP(tempEnv) = addr;
    longjmp (tempEnv, 1);
    printf("longjmp for stack\n");
  } 

  else { 
    printf("savedFunc called\n");
    savedFunc();
  }

  if (*(getOverrunPos()) != overRunFlag) 
    printf ("Stack overrun in executeOnTmpStack\n");
}

/* Replace Process */

void replaceImage (int srcRank, RAW_ADDR dataBottomAddr, RAW_ADDR dataTopAddr, RAW_ADDR stackBottomAddr, RAW_ADDR stackTopAddr)
{
  long dataLen;
  int start, x;
  MPI_Comm MPI_COMM_CMP_DUP, MPI_COMM_REP_DUP, CMP_REP_INTERCOMM_DUP;

  repRank = srcRank;

  /* Replace DATA */

  MPI_COMM_CMP_DUP = MPI_COMM_CMP;
  MPI_COMM_REP_DUP = MPI_COMM_REP;
  CMP_REP_INTERCOMM_DUP = CMP_REP_INTERCOMM;
  
  printf ("%p\t%p\t%p\n", &CMP_REP_INTERCOMM, &CMP_REP_INTERCOMM_DUP, CMP_REP_INTERCOMM); 

  struct heapListStruct *heapListHeadBackup = heapListHead, *heapListTailBackup = heapListTail;

  dataLen = dataTopAddr - dataBottomAddr + 1;

  for (start=0; start<dataLen; start+=1000)
  {
    if (dataLen - start < 1000)
      x = dataLen - start;

    else
      x = 1000;

    printf ("%p:\n", dataBottomAddr + start); 

    if (replace (dataBottomAddr + start, x, 51 + start, CMP_REP_INTERCOMM_DUP) < 0)
    {
      printf ("Replacing DATA Segment Unsuccessful!\n");
      PMPI_Abort (MPI_COMM_WORLD, 1);
    }
  }

  printf ("Replacing DATA Segment Successful!\n");

  MPI_COMM_CMP = MPI_COMM_CMP_DUP;
  MPI_COMM_REP = MPI_COMM_REP_DUP;
  CMP_REP_INTERCOMM = CMP_REP_INTERCOMM_DUP;

  stackSegAddr [0] = stackBottomAddr;
  stackSegAddr [1] = stackTopAddr;

  heapListHead = heapListHeadBackup;
  heapListTail = heapListTailBackup;

  /* Replace HEAP Data Structures */

  struct heapListStruct *heapData = heapListHead;

  int i = 0;
  printf("Starting heap replace while loop\n");
  while (heapData != NULL)
  {
    printf("Inside heap while\n");
    if (replace ((RAW_ADDR) heapData->ptr, heapData->size, 52 + i++, CMP_REP_INTERCOMM_DUP) < 0)
    {
      printf ("Replacing HEAP Data Structure Unsuccessful!\n");
      PMPI_Abort (MPI_COMM_WORLD, 1);
    }

    else
    {
      printf("memcpy for heap\n");
      memcpy (heapData->ptrAddr, &heapData->ptr, 8);
      printf("memcpy for heap1\n");
      heapData = heapData->next;
      printf("memcpy for heap2\n");
    }
  }

  /* Replace STACK */
  printf("Stack started\n");
  executeOnTmpStk (restoreStack);
}

/* Write Checkpoint */

int sendData (int destRank)
{
  int retVal, start, x;
  long dataLen, stackLen;
  //MPI_Request request;
  MPI_Status status;

  /* Send DATA Segment */

  dataLen = dataSegAddr [1] - dataSegAddr [0] + 1;

  printf ("Send Len = %d\n", dataLen);

  for (start = 0; start<dataLen; start+=1000)
  {
    if (dataLen - start < 1000)
      x = dataLen - start;

    else
      x = 1000;

    retVal = PMPI_Send ((void *) (dataSegAddr [0] + start), x, MPI_BYTE, destRank, 51+start, CMP_REP_INTERCOMM);
  
  }

  if (retVal != MPI_SUCCESS)
  { 
    printf ("Sending DATA Segment Unsuccessful!\n");
    return 1;
  }

  printf ("Sending DATA Segment Successful!\n");

  //PMPI_Wait (&request, &status); 

  /* Send HEAP Data Structures */

  struct heapListStruct *heapData = heapListHead;

  int i = 0;

  while (heapData != NULL)
  {
    retVal = PMPI_Send (heapData->ptr, heapData->size, MPI_BYTE, destRank, 52 + i++, CMP_REP_INTERCOMM);

    if (retVal != MPI_SUCCESS)
    {
      printf ("Sending HEAP Data Structure Unsuccessful!\n");
      return 1;
    }
   
    else
      heapData = heapData->next;
  }

  /* Send STACK Segment */
    
  stackLen = stackSegAddr [1] - stackSegAddr [0] + 1;

  retVal = PMPI_Send ((void *) stackSegAddr [0], stackLen, MPI_BYTE, destRank, 61, CMP_REP_INTERCOMM);
 
  if (retVal != MPI_SUCCESS)
  {
    printf ("Sending STACK Segment Unsuccessful!\n");
    return 1;
  }

  return 0;
}

/* Checkpoint Process */

void sendImage (int srcRank, int destRank)
{

  extern RAW_ADDR  _end, __data_start, _edata, __libc_setlocale_lock;
 
  if (setjmp (env) == 0 ) 
  {


   // Author: Abhishek
   RAW_ADDR rip;
   rip = env[0].__jmpbuf[7];
   PTR_DECRYPT(rip);
   env[0].__jmpbuf[7] = rip;


    printf ("sendImage 1\n");

    dataSegAddr [0] = ((RAW_ADDR) &__data_start & PAGE_MASK);

    printf ("sendImage 2\n");

    dataSegAddr [1] = ((RAW_ADDR) &_end & PAGE_MASK) + PAGE_SIZE - 1; 

    printf ("sendImage 3\n");

    stackSegAddr [0] = SPFromJmpBuf (env) & PAGE_MASK;
 
    printf ("stackSegAddr : %p\n", (void *) stackSegAddr [0]);

    printf ("sendImage 4\n");

    stackSegAddr [1] = env[0].__jmpbuf[1];
    PTR_DECRYPT(stackSegAddr[1]);
    //stackSegAddr [1] = (stackPosInit & PAGE_MASK) + PAGE_SIZE - 1;

    PMPI_Send (dataSegAddr, 2, MPI_UNSIGNED_LONG, destRank, 37, CMP_REP_INTERCOMM);
    printf ("sendImage 5\n");
    printf("%ld\n",MPI_UNSIGNED_LONG);

    printf("hsgdhfdh\n");
    PMPI_Send (stackSegAddr, 2, MPI_UNSIGNED_LONG, destRank, 38, CMP_REP_INTERCOMM);
    PMPI_Send (env, sizeof (jmp_buf), MPI_BYTE, destRank, 39, CMP_REP_INTERCOMM);
    printf ("ENV Size : %d\n", sizeof (jmp_buf));

    if (sendData (destRank) < 0)
    {
      printf ("RANK: %d. Sending Data Unsuccessful!\n", srcRank);
      PMPI_Abort (MPI_COMM_WORLD, 1);
    }
       
    longjmp (env, 1);

    printf ("You are not supposed to see this!\n");
  } 

  else
  {
    printf ("Checkpoint/Restart Successful!\n");
  }
}

/* Send Image */

void recvImage (int srcRank)
{
  MPI_Status status;
  PMPI_Recv (dataSegAddr, 2, MPI_UNSIGNED_LONG, srcRank, 37, CMP_REP_INTERCOMM, &status);
  PMPI_Recv (stackSegAddr, 2, MPI_UNSIGNED_LONG, srcRank, 38, CMP_REP_INTERCOMM, &status);
  PMPI_Recv (env, sizeof (jmp_buf), MPI_BYTE, srcRank, 39, CMP_REP_INTERCOMM, &status);

  printf ("StackAddr %p\n", (void *) stackSegAddr [0]);
  replaceImage (srcRank, dataSegAddr [0], dataSegAddr [1], stackSegAddr [0], stackSegAddr [1]);

  printf ("You are not supposed to see this!\n");
}
  
void repManager ()
{

  /*FILE *p = fopen ("/proc/self/maps", "r");
  char *line = (char *) malloc (1000 * sizeof (char));

  line = fgets (line, 1000, p);
  while (line != NULL)
  {
    printf ("%s", line);
    line = fgets (line, 1000, p);
  }*/

  int i, c, r, localRank, destRank = -1, srcRank = -1;
  char fileName [100];
  sprintf (fileName, "%s/%s", workDir, repState);

  FILE *fp = fopen (fileName, "r");

  if (MPI_COMM_REP == MPI_COMM_NULL)
    PMPI_Comm_rank (MPI_COMM_CMP, &localRank);
  
  else
    PMPI_Comm_rank (MPI_COMM_REP, &localRank);

  for (i=0; i<nC; i++)
  {
    if (i == nC - 1)
      fscanf (fp, "%d\t%d", &c, &r);

    else
      fscanf (fp, "%d\t%d\n", &c, &r);

    printf("c: %d r: %d\n", c, r);

    if (r != -1) 
    {
      if (MPI_COMM_REP == MPI_COMM_NULL)
      {
        if (localRank == c && cmpToRepMap [localRank] != r)
          destRank = r;
      }
 
      else
      {
        if (localRank == r && cmpToRepMap [c] != localRank)
          srcRank = c;
      }
   
      repToCmpMap [r] = c;
    }
   
    cmpToRepMap [c] = r;
  }
 
  fclose (fp);

  printf("LocalRank: %d Dest: %d Src: %d\n", localRank, destRank, srcRank);
 
  double start_time = MPI_Wtime();

  if (destRank != -1)
  {
    printf ("%d Sending image to rank %d BEGIN\n", localRank, destRank);
    sendImage (localRank, destRank);
    printf ("%d Sending image to rank %d END\n", localRank, destRank);
  }

  else if (srcRank != -1)
  {
    printf ("Receiving image from rank %d BEGIN\n", srcRank);
    recvImage (srcRank);
    printf ("Receiving image from rank %d END\n", srcRank);
  }

  PMPI_Barrier (MPI_COMM_WORLD);

  if (localRank == 0 && MPI_COMM_REP == MPI_COMM_NULL)
  {
    double end_time = MPI_Wtime();
    printf ("Replica Change Time (s) = %f\n", end_time - start_time);
    MPI_Abort (MPI_COMM_WORLD, 1);
  }
}

/* MPI_Init */

int MPI_Init (int *argc, char ***argv)
{
  jmp_buf tempEnv;
  int i, retVal, worldRank, worldSize, color, cmpLeader, repLeader;

  retVal = PMPI_Init (argc, argv);

  // #define free myFree

  if (setjmp (tempEnv) == 0)
  {
    stackPosInit = SPFromJmpBuf (tempEnv);
    longjmp (tempEnv, 1);
  }
  
  printf("Jmp_buf content: %p", tempEnv[0].__jmpbuf[1]);

  PMPI_Comm_rank (MPI_COMM_WORLD, &worldRank);
  PMPI_Comm_size (MPI_COMM_WORLD, &worldSize);

  nR = worldSize - (int)(worldSize / REP_DEGREE);
  nC = worldSize - nR;

  printf("Compute Nodes: %d | Replication Nodes: %d\n", nC, nR);

  if (nR > 0)
  {

    cmpToRepMap = (int *) myMalloc (&cmpToRepMap, nC * sizeof (int));
    repToCmpMap = (int *) myMalloc (&repToCmpMap, nR * sizeof (int));
  
    for (i=0; i<nC; i++)
    {
      if (i < nR)
      {
        cmpToRepMap [i] = -1;
        repToCmpMap [i] = -1;
      }
   
      else
        cmpToRepMap [i] = -1;
    }

    MPI_COMM_CMP = MPI_COMM_REP = CMP_REP_INTERCOMM = MPI_COMM_NULL;

    color = worldRank < nC ? 0 : 1;
    cmpLeader = 0;
    repLeader = nC;
 

    printf("WorldRank: %d Color: %d\n", worldRank, color);

 
    if (color == 0)
    {
      PMPI_Comm_split (MPI_COMM_WORLD, color, 0, &MPI_COMM_CMP);
      PMPI_Intercomm_create (MPI_COMM_CMP, 0, MPI_COMM_WORLD, repLeader, 1, &CMP_REP_INTERCOMM);
      
      int tRank;
      PMPI_Comm_rank(MPI_COMM_CMP, &tRank);
      printf("WorldRank: %d My Rank: %d\n",worldRank, tRank);
    }

    else if (color == 1)
    {
      printf("color 1\n");
      PMPI_Comm_split (MPI_COMM_WORLD, color, 0, &MPI_COMM_REP);
      
      printf("after split\n");
      PMPI_Intercomm_create (MPI_COMM_REP, 0, MPI_COMM_WORLD, cmpLeader, 1, &CMP_REP_INTERCOMM);
    
       
      int tRank;
      PMPI_Comm_rank(MPI_COMM_REP, &tRank);
      printf("WorldRank: %d My Rank: %d\n",worldRank, tRank);

    }
  }


  return retVal;
}

/* MPI_Comm_rank */

int MPI_Comm_rank (MPI_Comm comm, int *rank)
{
  int myRank;

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    if (MPI_COMM_CMP == MPI_COMM_NULL)
    {
      PMPI_Comm_rank (MPI_COMM_REP, &myRank);
      *rank = repToCmpMap [myRank];
      return MPI_SUCCESS;
    }

    else
      return PMPI_Comm_rank (MPI_COMM_CMP, rank);
  }
  
  else
    return PMPI_Comm_rank (comm, rank);
}

// MPI_Comm_size 

int MPI_Comm_size (MPI_Comm comm, int *size)
{
  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    repManager ();
 
    if (MPI_COMM_CMP == MPI_COMM_NULL)
      return PMPI_Comm_remote_size (CMP_REP_INTERCOMM, size);

    else
      return PMPI_Comm_size (MPI_COMM_CMP, size);
  }

  else
    return PMPI_Comm_size (comm, size);
}

// MPI_Send 

int MPI_Send (const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  int destRep, retVal;

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    if (MPI_COMM_CMP == MPI_COMM_NULL)
    {
      retVal = PMPI_Send (buf, count, datatype, dest, tag, CMP_REP_INTERCOMM);

      destRep = cmpToRepMap [dest];
      if (destRep != -1)
        retVal = PMPI_Send (buf, count, datatype, destRep, tag, MPI_COMM_REP);
    }

    else
    {
      retVal = PMPI_Send (buf, count, datatype, dest, tag, MPI_COMM_CMP);

      destRep = cmpToRepMap [dest];
      if (destRep != -1)
        retVal = PMPI_Send (buf, count, datatype, destRep, tag, CMP_REP_INTERCOMM);
    }

    return retVal;
  }

  else
    return PMPI_Send (buf, count, datatype, dest, tag, comm);
}

// MPI_Isend 

int MPI_Isend (const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int destRep, retVal, typeSize;
  struct MPIReqStruct *reqPair;
  PMPI_Type_size (datatype, &typeSize);

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    if (MPI_COMM_CMP == MPI_COMM_NULL)
    {
      retVal = PMPI_Isend (buf, count, datatype, dest, tag, CMP_REP_INTERCOMM, request);

      destRep = cmpToRepMap [dest];
      if (destRep != -1)
      {
        reqPair = createMR (request);
        reqPair->reqRep = (MPI_Request *) myMalloc (&(reqPair->reqRep), sizeof (MPI_Request));
        reqPair->reqRepBuf = (void*) myMalloc (&(reqPair->reqRepBuf), count * typeSize);
        PMPI_Isend (reqPair->reqRepBuf, count, datatype, destRep, tag, MPI_COMM_REP, reqPair->reqRep);
      }
    }

    else
    {
      retVal = PMPI_Isend (buf, count, datatype, dest, tag, MPI_COMM_CMP, request);

      destRep = cmpToRepMap [dest];
      if (destRep != -1)
      {
        reqPair = createMR (request);
        reqPair->reqRep = (MPI_Request *) myMalloc (&(reqPair->reqRep), sizeof (MPI_Request));
        reqPair->reqRepBuf = (void*) myMalloc (&(reqPair->reqRepBuf), count * typeSize);
        PMPI_Isend (reqPair->reqRepBuf, count, datatype, destRep, tag, CMP_REP_INTERCOMM, reqPair->reqRep);
      }
    }

    return retVal;
  }

  else
    return PMPI_Isend (buf, count, datatype, dest, tag, comm, request);
}


// MPI_Recv

int MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int srcRep, retVal, rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {

    if (MPI_COMM_CMP == MPI_COMM_NULL)
    {
      if (source == MPI_ANY_SOURCE)
        PMPI_Recv (&source, 1, MPI_INT, rank, 27, CMP_REP_INTERCOMM, status);

      retVal = PMPI_Recv (buf, count, datatype, source, tag, CMP_REP_INTERCOMM, status);

      srcRep = cmpToRepMap [source];
      if (srcRep != -1)
        retVal = PMPI_Recv (buf, count, datatype, srcRep, tag, MPI_COMM_REP, status);
    }

    else
    {
      retVal = PMPI_Recv (buf, count, datatype, source, tag, MPI_COMM_CMP, status);

      if (source == MPI_ANY_SOURCE && cmpToRepMap [rank] != -1)
      {
        source = (*status).MPI_SOURCE;
        PMPI_Send (&source, 1, MPI_INT, cmpToRepMap [rank], 27, CMP_REP_INTERCOMM);
      }

      srcRep = cmpToRepMap [source];
      if (srcRep != -1)
        retVal = PMPI_Recv (buf, count, datatype, srcRep, tag, CMP_REP_INTERCOMM, status);
    }

    return retVal;
  }
 
  else
    return PMPI_Recv (buf, count, datatype, source, tag, comm, status);
}

// MPI_Irecv 

int MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  int srcRep, retVal, typeSize, rank;
  struct MPIReqStruct *reqPair;
  MPI_Status status;
  PMPI_Type_size (datatype, &typeSize);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    if (MPI_COMM_CMP == MPI_COMM_NULL)
    {
      if (source == MPI_ANY_SOURCE)
        PMPI_Recv (&source, 1, MPI_INT, rank, 28, CMP_REP_INTERCOMM, &status);
    
      retVal = PMPI_Irecv (buf, count, datatype, source, tag, CMP_REP_INTERCOMM, request);

      srcRep = cmpToRepMap [source];
      if (srcRep != -1)
      {
        reqPair = createMR (request);
        reqPair->reqRep = (MPI_Request *) myMalloc (&(reqPair->reqRep), sizeof (MPI_Request));
        reqPair->reqRepBuf = (void*) myMalloc (&(reqPair->reqRepBuf), count * typeSize);
        PMPI_Irecv (reqPair->reqRepBuf, count, datatype, srcRep, tag, MPI_COMM_REP, reqPair->reqRep);
      }
    }

    else
    {
      retVal = PMPI_Irecv (buf, count, datatype, source, tag, MPI_COMM_CMP, request);

      if (source == MPI_ANY_SOURCE && cmpToRepMap [rank] != -1)
      {
        PMPI_Wait (request, &status);
        source = status.MPI_SOURCE;
        PMPI_Send (&source, 1, MPI_INT, cmpToRepMap [rank], 28, CMP_REP_INTERCOMM);
      }

      srcRep = cmpToRepMap [source];
      if (srcRep != -1)
      {

        reqPair = createMR (request);
        reqPair->reqRep = (MPI_Request *) myMalloc (&(reqPair->reqRep), sizeof (MPI_Request));
        reqPair->reqRepBuf = (void*) myMalloc (&(reqPair->reqRepBuf), count * typeSize);
        PMPI_Irecv (reqPair->reqRepBuf, count, datatype, srcRep, tag, CMP_REP_INTERCOMM, reqPair->reqRep);
      }
    }

    return retVal;
  }

  else
    return PMPI_Irecv (buf, count, datatype, source, tag, comm, request);
}

// MPI_Wait 

int MPI_Wait (MPI_Request *request, MPI_Status *status)
{
  struct MPIReqStruct *reqPair;

  if (nR > 0)
  {
    reqPair = findMR (request);
   
    if (reqPair != NULL)
    {
      PMPI_Wait (reqPair->reqRep, status);
      PMPI_Wait (request, status);
      myFree (reqPair->reqRep);
      myFree (reqPair->reqRepBuf);
      deleteMR (request);
    }
     
    else
      PMPI_Wait (request, status);
  }   
  
  else
    PMPI_Wait (request, status);
}

// MPI_Reduce 

int MPI_Reduce (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int rank, src, srcRep, rootRep, size, i;
  MPI_Status status;

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    //repManager ();

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &size);

    if (rank == root)
    {
      if (datatype == MPI_INT || datatype == MPI_INTEGER)
      {
        int buf [count], result [count]; 
        result [0] = -1;

        for (src=0; src<size; src++)
        {
          if (src != rank)
            MPI_Recv (buf, count, datatype, src, 23, MPI_COMM_WORLD, &status);

          else
            buf [0] = *(int *)sendbuf;

          if (result [0] == -1)
          {
            for (i=0; i<count; i++)
              result [i] = buf [i];
          }

          else if (op == MPI_MIN)
          {
            for (i=0; i<count; i++)
              if (buf [i] < result [i])
                result [i] = buf [i];
          }

          else if (op == MPI_MAX)
          {
            for (i=0; i<count; i++)
              if (buf [i] > result [i])
                result [i] = buf [i];
          }
 
          else if (op == MPI_SUM)
          {
            for (i=0; i<count; i++)
              result [i] += buf [i];
          }

          else if (op == MPI_PROD)
          {
            for (i=0; i<count; i++)
              result [i] *= buf [i];
          }
    
          else 
          {
            printf ("Reduce operation not implemented by prodiling layer!\n");
            PMPI_Abort (MPI_COMM_WORLD, 1);
          }
        }
 
        for (i=0; i<count; i++)
          ((int *) recvbuf) [i] = result [i];
      }
  
      else if (datatype == MPI_DOUBLE_PRECISION || datatype == MPI_DOUBLE)
      {
        double buf [count], result [count];
        result [0] = -1;

        for (src=0; src<size; src++)
        {
          if (src != rank)
            MPI_Recv (buf, count, datatype, src, 23, MPI_COMM_WORLD, &status);

          else
            buf [0] = *(double *) sendbuf;

          if (result [0] == -1)
          {
            for (i=0; i<count; i++)
              result [i] = buf [i];
          }

          else if (op == MPI_MIN)
          {
            for (i=0; i<count; i++)
              if (buf [i] < result [i])
                result [i] = buf [i];
          }

          else if (op == MPI_MAX)
          {
            for (i=0; i<count; i++)
              if (buf [i] > result [i])
                result [i] = buf [i];
          }
 
          else if (op == MPI_SUM)
          {
            for (i=0; i<count; i++)
              result [i] += buf [i];
          }

          else if (op == MPI_PROD)
          {
            for (i=0; i<count; i++)
              result [i] *= buf [i];
          }
  
          else 
          {
            printf ("Reduce operation not implemented by prodiling layer!\n");
            PMPI_Abort (MPI_COMM_WORLD, 1);
          }
        }
 
        for (i=0; i<count; i++)
          ((double *) recvbuf) [i] = result [i];  
      }

      else 
      {
        printf ("Data Type Not Defined!\n");
        PMPI_Abort (MPI_COMM_WORLD, 1);
      }
    }
          
    else
      MPI_Send (sendbuf, count, datatype, root, 23, MPI_COMM_WORLD);

    return MPI_SUCCESS;
  }
  
  else
    return PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, root, comm);
}

// MPI_Allreduce 

int MPI_Allreduce (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int rank, size, dest, root = 0;
  MPI_Status status;

  if (nR > 0 && comm == MPI_COMM_WORLD)
  {

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &size);

    MPI_Reduce (sendbuf, recvbuf, count, datatype, op, root, comm);
  
    if (rank == root)
    {
      for (dest=0; dest<size; dest++)
      {
        if (dest != rank)
          MPI_Send (recvbuf, count, datatype, dest, 24, MPI_COMM_WORLD);
      }
    }

    else
      MPI_Recv (recvbuf, count, datatype, root, 24, MPI_COMM_WORLD, &status);
  
    return MPI_SUCCESS;
  } 
  
  else
    return PMPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm);
}

// MPI_Alltoall 


// MPI_Alltoallv 



int MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int rank, size, dest;
  MPI_Status status;
  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  
  if (nR > 0 && comm == MPI_COMM_WORLD)
  {
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &size);
   
    if (rank == root)
    {
      for (dest=0; dest<size; dest++)
      {
        if (dest != rank)
          MPI_Send (buffer, count, datatype, dest, 25, MPI_COMM_WORLD);
      }
    }
 
    else
      MPI_Recv (buffer, count, datatype, root, 25, MPI_COMM_WORLD, &status);
  
    return MPI_SUCCESS;
  }

  else
    return PMPI_Bcast (buffer, count, datatype, root, comm);
}
