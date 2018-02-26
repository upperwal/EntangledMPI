#!/bin/bash

AC_PREREQ([2.68])
AC_INIT([empi], [0.1], [a.upperwal@gmail.com])
AC_CONFIG_AUX_DIR([build_aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# prevent setting xFLAGS to default of -g -O2
# https://ubuntuforums.org/showthread.php?t=1556520
if test x"$CFLAGS" = x""; then
  AC_SUBST(CFLAGS, [ ])
fi
if test x"$CXXFLAGS" = x""; then
  AC_SUBST(CXXFLAGS, [ ])
fi

AM_PROG_CC_C_O

AM_PROG_AR
LT_INIT


AC_CONFIG_MACRO_DIRS([m4])

AC_CONFIG_SRCDIR([src/manager/main.c])
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

# https://stackoverflow.com/questions/32192031/makefile-am-configure-ac-conditional-environment-variable-check
AC_ARG_ENABLE(stack-protector,[AS_HELP_STRING([--disable-stack-protector], [Disable Stack Protector])])
AC_MSG_CHECKING([whether to disable stack protector])
AS_IF([test "x${enable_stack_protector}" = "xno" ], AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]))
AM_CONDITIONAL([DISABLESPROT],[test "x${enable_stack_protector}" = "xno"])

if test "x${enable_stack_protector}" = "xno"; then
	AC_SUBST([REP_CFLAGS], ['-fno-stack-protector'])
fi


# Checks for programs.
#CC=${CC-/usr/bin/mpicc}
AC_ARG_VAR([CC],[the 'diff' program to use for test output comparison])
AC_PATH_PROG([CC], [mpicc], [cc], [/usr/bin])
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
