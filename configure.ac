#!/bin/bash

AC_PREREQ([2.68])
AC_INIT([fmpi], [1.0], [chengkai.me@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_PROG_AR
LT_INIT

AC_CONFIG_SRCDIR([src/manager/main.c])
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/manager/Makefile
	src/replication/Makefile
])



# Checks for programs.
#CC=${CC-/usr/bin/mpicc}
AC_ARG_VAR([CC],[the 'diff' program to use for test output comparison])
AC_PATH_PROG([CC], [mpicc], [/usr/bin/mpicc], [/usr/bin])
#AC_PROG_CC(MPICC)
#AX_MPI

#AX_PROG_CC_MPI()
#export CC = mpicc

#CC = "mpicc"

#set CC mpicc

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT