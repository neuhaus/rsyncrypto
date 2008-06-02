#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([rsyncrypto],[1.10])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([rsyncrypto.h])
AM_CONFIG_HEADER([config.h])
#AM_MAINTAINER_MODE
AC_COPYRIGHT([Copyright (C) 2005-2008 Shachar Shemesh for Lingnu Open Source Consulting ltd.])

# Checks for programs.
AC_PROG_CXX
AC_LANG(C++)
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_AWK

# Checks for libraries.
dnl Checks for openssl
AC_ARG_WITH([openssl],AC_HELP_STRING([--with-openssl=DIR],[use openssl install directory]),
[AC_MSG_CHECKING(for openssl path override)
case "$withval" in
  yes|no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT($withval)
    CPPFLAGS="$CPPFLAGS -I$withval/include"

    LDFLAGS="$LDFLAGS -L$withval/lib"
    ;;
  esac])
AC_CHECK_LIB([crypto], [AES_encrypt],,[AC_MSG_FAILURE([AES implementation in openssl not found])])

dnl Checks for libargtable2
AC_ARG_WITH([argtable2],AC_HELP_STRING([--with-argtable2=DIR],[use argtable2 build directory]),
[AC_MSG_CHECKING(for argtable2 path override)
case "$withval" in
  yes|no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT($withval)

    CPPFLAGS="$CPPFLAGS -I$withval"
    LDFLAGS="$LDFLAGS -L$withval/.libs"

  esac])

AC_CHECK_LIB([argtable2], [arg_parse],,[AC_MSG_FAILURE([argtable2 not found])])

AC_MSG_CHECKING([whether platform supports lstat])
AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY([lstat])],
        [AC_MSG_RESULT(yes)
        AC_DEFINE([HAVE_LSTAT],1,[Define to 1 if your platform has the "lstat" function call])],
        [AC_MSG_RESULT(no)])

# Checks for header files.
AC_SYS_LARGEFILE
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h netinet/in.h stdlib.h string.h strings.h sys/time.h unistd.h utime.h sys/mman.h])

AC_CHECK_HEADERS([argtable2.h] [map],[],[AC_MSG_FAILURE([Mandatory header file not found])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

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

AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_STAT
AC_CHECK_FUNCS([bzero dup2 memset mkdir munmap rmdir strchr strerror strtoul])

dnl Define a few necessary stuff
AC_DEFINE([DIRSEP_S], ["/"], [Directory separator as string])
AC_DEFINE([DIRSEP_C], ['/'], [Directory separator as char])

dnl Find out whether we have a "rsyncable" supporting gzip
AC_MSG_CHECKING([for gzip with "rsyncable" support])
if ([ gzip --rsyncable >/dev/null 2>conftest.out </dev/null && [ "$(ls -l conftest.out | tr -s ' ' | cut -d ' ' -f 5)" = 0 ] ])
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
AC_OUTPUT
