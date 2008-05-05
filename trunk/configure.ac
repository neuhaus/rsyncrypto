#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([rsyncrypto],[1.06])
AM_INIT_AUTOMAKE
#AM_MAINTAINER_MODE
AC_COPYRIGHT([Copyright (C) 2005-2007 Shachar Shemesh for Lingnu Open Source Consulting ltd.])

# Checks for programs.
AC_PROG_CXX
AC_LANG(C++)

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
AC_CHECK_LIB(crypto, AES_encrypt,,[AC_MSG_FAILURE([AES implementation in openssl not found])])

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

AC_CHECK_LIB(argtable2, arg_parse,,[AC_MSG_FAILURE([argtable2 not found])])

AC_MSG_CHECKING([whether platform supports lstat])
AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY([lstat])],
        [AC_MSG_RESULT(yes)
        AC_DEFINE([HAVE_LSTAT],1,[Define to 1 if your platform has the "lstat" function call])],
        [AC_MSG_RESULT(no)])

# Checks for header files.
AC_SYS_LARGEFILE
AC_CHECK_HEADERS([argtable2.h])

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

dnl Check which std::map variant to use
AC_LANG_PUSH(C++)
AC_LANG_WERROR

AC_MSG_CHECKING([whether compiler supports the upcoming ISO C++ standard])
saved_CXXFLAGS=$CXXFLAGS
CXXFLAGS="$CXXFLAGS -std=c++0x"
AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([
        #include <stdio.h>
        ],
        [ printf("Hello, world\n"); ])],
    [
        AC_MSG_RESULT([yes])
    ],
    [
        AC_MSG_RESULT([no])
        CXXFLAGS="$saved_CXXFLAGS"
    ]
)

AC_MSG_CHECKING([for std::map])
AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([
        #include <unordered_map>
        ],
        [ std::unordered_map<int, int> test; ])],
    [
        AC_MSG_RESULT([std::unordered_map])
        AC_DEFINE([MAP_INCLUDE], [<unordered_map>], [The file to include in order to get the map class])
        AC_DEFINE([MAP_CLASS], [std::unordered_map], [The type for the map class])
    ],
    [
        AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([
            #include <ext/hash_map>
        ],
        [ __gnu_cxx::hash_map<int,int> test ])],
    [
        AC_MSG_RESULT([__gnu_cxx::hash_map])
        AC_DEFINE([MAP_INCLUDE], [<ext/hash_map>])
        AC_DEFINE([MAP_CLASS], [__gnu_cxx::hash_map])
    ],
    [
        AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([
            #include <map>
        ],
        [ std::map<int,int> test ])],
    [
        AC_MSG_RESULT([std::map])
        AC_DEFINE([MAP_INCLUDE], [<map>])
        AC_DEFINE([MAP_CLASS], [std::map])
    ],
    [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([Without a working map class this program cannot be compiled!])
    ])])]
)

AC_LANG_POP

# Checks for library functions.
AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)
AC_OUTPUT
