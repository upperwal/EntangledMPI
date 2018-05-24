#include <mpi.h>

#ifdef OPEN_MPI
#include <mpi-ext.h>
#endif

#include "src/misc/log.h"

// MPI wrapper functions for fortran
// This is a non standard way to do this (ie. appending "_" to the end of fortran functions)
// Different fortran compilers do different things like "_" or "__" or something else
// This works in Xeon Phi cluster of Mars Lab.

#define MPI_Fint int

void mpi_init_(MPI_Fint *ierr) {
	debug_log_i("[Fortran] MPI_Init");
	MPI_Init(NULL, NULL);
}

void mpi_finalize_(MPI_Fint *ierr) {
	MPI_Finalize();
	debug_log_i("[Fortran] MPI_Finalize");
}

void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr) {
    int ierr_c;
    MPI_Comm c_comm;

    c_comm = PMPI_Comm_f2c(*comm);

    ierr_c = MPI_Barrier(c_comm);
    //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(ierr_c);
}

void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr) {
	int c_ierr;
	debug_log_i("Rank");
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    debug_log_i("Doing Rank");
    c_ierr = MPI_Comm_rank(c_comm, rank);

    //*ierr = c_ierr;
    debug_log_i("Rank end %d", *rank);
}

void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr) {
	int c_ierr;
    MPI_Comm c_comm = PMPI_Comm_f2c( *comm );

    c_ierr = MPI_Comm_size(c_comm, size);
}

void mpi_send_(char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr) {
	int c_ierr;

    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Datatype c_type = PMPI_Type_f2c(*datatype);

    c_ierr = MPI_Send(buf, *count, c_type, *dest, *tag, c_comm);

    //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);
}

void mpi_recv_(char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr) {
   MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
   MPI_Datatype c_type = PMPI_Type_f2c(*datatype);
   int c_ierr;

   /* Call the C function */
   c_ierr = MPI_Recv(buf, *count, c_type, *source, *tag, c_comm, status);
   //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);
}

void mpi_scatter_(char *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, char *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
	int c_ierr;
    MPI_Datatype c_sendtype, c_recvtype;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

    c_sendtype = PMPI_Type_f2c(*sendtype);
    c_recvtype = PMPI_Type_f2c(*recvtype);

    c_ierr = MPI_Scatter(sendbuf,*sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
    //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);
}

void mpi_bcast_(char *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
	int c_ierr;
    MPI_Comm c_comm;
    MPI_Datatype c_type;

    c_comm = PMPI_Comm_f2c(*comm);
    c_type = PMPI_Type_f2c(*datatype);

    debug_log_i("[Fortran] Rank: %d", *root);
    c_ierr = MPI_Bcast(buffer, *count, c_type, *root, c_comm);
    //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);
}

void mpi_gather_(char *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, char *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
	int c_ierr;
    MPI_Comm c_comm;
    MPI_Datatype c_sendtype, c_recvtype;

    c_comm = PMPI_Comm_f2c(*comm);
    c_sendtype = PMPI_Type_f2c(*sendtype);
    c_recvtype = PMPI_Type_f2c(*recvtype);

    c_ierr = MPI_Gather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
    //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);
}

void mpi_allgather_(char *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, char *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr) {
	int ierr_c;
    MPI_Comm c_comm;
    MPI_Datatype c_sendtype, c_recvtype;

    c_comm = PMPI_Comm_f2c(*comm);
    c_sendtype = PMPI_Type_f2c(*sendtype);
    c_recvtype = PMPI_Type_f2c(*recvtype);

    ierr_c = MPI_Allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);

    //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(ierr_c);
}

void mpi_reduce_(char *sendbuf, char *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) {
    int c_ierr;
    MPI_Datatype c_type;
    MPI_Op c_op;
    MPI_Comm c_comm;

    c_type = PMPI_Type_f2c(*datatype);
    c_op = PMPI_Op_f2c(*op);
    c_comm = PMPI_Comm_f2c(*comm);

    c_ierr = MPI_Reduce(sendbuf, recvbuf, *count, c_type, c_op, *root, c_comm);
   //if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);
}

void mpi_allreduce_(char *sendbuf, char *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) {
	int ierr_c;
    MPI_Comm c_comm;
    MPI_Datatype c_type;
    MPI_Op c_op;

    c_comm = PMPI_Comm_f2c(*comm);
    c_type = PMPI_Type_f2c(*datatype);
    c_op = PMPI_Op_f2c(*op);

    ierr_c = MPI_Allreduce(sendbuf, recvbuf, *count, c_type, c_op, c_comm);
    //if (NULL != ierr) *ierr = ierr_c;
}

void mpi_isend_(char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
    int c_ierr;
    MPI_Datatype c_type = PMPI_Type_f2c(*datatype);
    MPI_Request c_req;
    MPI_Comm c_comm;

    c_comm = PMPI_Comm_f2c (*comm);

    c_ierr = MPI_Isend(buf, *count, c_type, *dest, *tag, c_comm, &c_req);
    if (NULL != ierr) *ierr = c_ierr;

    if (MPI_SUCCESS == c_ierr) {
      *request = c_req;
    }
}

void mpi_irecv_(char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr) {
    int c_ierr;
    MPI_Datatype c_type = PMPI_Type_f2c(*datatype);
    MPI_Request c_req;
    MPI_Comm c_comm;

    c_comm = PMPI_Comm_f2c (*comm);

    c_ierr = MPI_Irecv(buf, *count, c_type, *source, *tag, c_comm, &c_req);
    if (NULL != ierr) *ierr = c_ierr;

    if (MPI_SUCCESS == c_ierr) {
      *request = c_req;
    }
    debug_log_i("*request: %p | c_req: %p", *request, c_req);
}

void mpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr) {
    int c_ierr;
    MPI_Request c_req = *request;
    MPI_Status  c_status;

    c_ierr = MPI_Wait(&c_req, &c_status);
    if (NULL != ierr) *ierr = c_ierr;
}
