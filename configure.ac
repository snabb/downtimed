#
# configure.ac
#
# Copyright (c) 2009-2011 EPIPE Communications. All rights reserved.
#
# This software is licensed under the terms and conditions of the FreeBSD
# License which is also known as the Simplified BSD License. You should have 
# received a copy of that license along with this software.
# 

AC_PREREQ(2.64)
AC_INIT([downtimed],[0.5],[opensource@epipe.com],[],[http://dist.epipe.com/downtimed/])
AC_CONFIG_SRCDIR([downtimed.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign dist-xz])
            
AC_LANG([C])
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_PROG_INSTALL
AC_PROG_SED
AC_PROG_MAKE_SET

AC_C_INLINE
AC_C_BIGENDIAN

AC_CHECK_HEADERS([sys/param.h])

# check sys/sysctl.h seperately, as it requires other headers on OpenBSD
AC_CHECK_HEADERS([sys/sysctl.h], [], [],
[[#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
]])

AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AH_TEMPLATE([sig_atomic_t], [Define to int if <signal.h> does not define.])
AC_CHECK_TYPES([sig_atomic_t], [], [AC_DEFINE([sig_atomic_t], [int])], [
#include <signal.h>
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# eof
