#
# configure.ac
#
# Copyright (c) 2009-2010 EPIPE Communications. All rights reserved.
#
# This software is licensed under the terms and conditions of the FreeBSD
# License which is also known as the Simplified BSD License. You should have 
# received a copy of that license along with this software.
# 

AC_PREREQ(2.64)
AC_INIT([downtimed],[0.2],[opensource@epipe.com],[],[http://dist.epipe.com/downtimed/])
AC_CONFIG_SRCDIR([downtimed.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign dist-xz])
            
AC_PROG_CC
AC_LANG([C])
AM_PROG_CC_C_O

AC_PROG_INSTALL
AC_PROG_SED
AC_PROG_MAKE_SET

AC_CHECK_HEADERS([sys/endian.h endian.h sys/sysctl.h])

AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# eof
