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

# Checks for library functions.
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADER([config.h])
AC_OUTPUT
