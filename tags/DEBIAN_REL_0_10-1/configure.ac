#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([rsyncrypt],[0.10])
AM_INIT_AUTOMAKE
AC_COPYRIGHT([Copyright (C) 2005 Shachar Shemesh for Lingnu Open Source Consulting ltd.])

# Checks for programs.
AC_PROG_CXX
AC_LANG(C++)

# Checks for libraries.
AC_CHECK_LIB(crypto, AES_encrypt,,[AC_MSG_FAILURE([AES implementation in openssl not found])])
        
# Checks for header files.
AC_SYS_LARGEFILE
AC_CHECK_HEADERS()

# Checks for typedefs, structures, and compiler characteristics.
AC_MSG_CHECKING([for O_NOATIME support in open])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>],
            [int fd=open("/dev/null", O_RDONLY|O_NOATIME)])],
        [AC_DEFINE([HAVE_NOATIME],[1],[Define to 1 if "open" supports O_NOATIME])
        AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])
            
AC_MSG_CHECKING([whether struct stat has nanosecond resolution])
AC_COMPILE_IFELSE([ dnl check for Linux st_mtim
        AC_LANG_PROGRAM([[#include <sys/stat.h>]],[[struct stat st; st.st_mtim.tv_nsec=0;]])],
        [AC_DEFINE([HAVE_STAT_NSEC],[1], [Define to 1 if struct stat supports nanosecond resolution])
        AC_DEFINE([st_mtime_nsec],[st_mtim.tv_nsec],[mtime nsec access replacement])
        AC_DEFINE([st_atime_nsec],[st_atim.tv_nsec],[atime nsec access replacement])
        AC_DEFINE([st_ctime_nsec],[st_ctim.tv_nsec],[ctime nsec access replacement])
        AC_MSG_RESULT([st_mtim is a struct timespec])],
        [ dnl check for FreeBSD st_mtimespec
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>]],[[struct stat st; st.st_mtimespec.tv_nsec=0;]])],
            [AC_DEFINE([HAVE_STAT_NSEC],[1])
            AC_DEFINE([st_mtime_nsec],[st_mtimespec.tv_nsec])
            AC_DEFINE([st_atime_nsec],[st_atimespec.tv_nsec])
            AC_DEFINE([st_ctime_nsec],[st_ctimespec.tv_nsec])
            AC_MSG_RESULT([st_mtimespec is a struct timespec])],
            [AC_MSG_RESULT([no])
            AC_MSG_WARN([file times will have a 1 second resolution])])])

# Find out whether we have a "rsyncable" supporting gzip
AC_MSG_CHECKING([for gzip with "rsyncable" support])
if ( gzip --rsyncable >/dev/null </dev/null )
then
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no!])
    AC_MSG_WARN([!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!])
    AC_MSG_WARN([! gzip must support the "--rsyncable" flag for proper rsyncrypto operation.         !])
    AC_MSG_WARN([! This does not affect compilation, but a proper gzip must be available at runtime. !])
    AC_MSG_WARN([!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!])
fi
# Checks for library functions.
AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)
AC_OUTPUT
