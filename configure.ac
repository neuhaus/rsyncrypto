#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([rsyncrypt],[0.01])
AC_COPYRIGHT([Copyright (C) 2005 Shachar Shemesh for Lingnu Open Source Consulting ltd.])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB(crypto, AES_encrypt,,[AC_MSG_FAILURE([AES implementation in openssl not found])])
        
# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_LANG(C++)
AC_MSG_CHECKING([whether struct stat has nanosecond resolution])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>]],[[struct stat st; st.st_mtim.tv_nsec=0;]])]
        ,[AC_DEFINE([HAVE_STAT_NSEC],[1], [struct stat has nanosecond resolution])
        AC_DEFINE([st_mtime_nsec],[st_mtim.tv_nsec],[mtime nsec access replacement])
        AC_DEFINE([st_atime_nsec],[st_atim.tv_nsec],[atime nsec access replacement])
        AC_DEFINE([st_ctime_nsec],[st_ctim.tv_nsec],[ctime nsec access replacement])
        AC_MSG_RESULT([st_mtim is a struct timeval])],
        [AC_MSG_RESULT([no])
        AC_MSG_WARN([file times will have a 1 second resolution])])

# Checks for library functions.
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADER([config.h])
AC_OUTPUT
