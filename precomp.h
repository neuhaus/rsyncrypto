// Precompiled headers file
#ifndef PRECOMPILED_HEADERS_H
#define PRECOMPILED_HEADERS_H

#include "config.h"

// Windows all encompasing includes
#if defined(_WIN32)

// Override the Windows definition of off_t
typedef unsigned long long _off_t;
typedef _off_t off_t;
#define _OFF_T_DEFINED

#include <tchar.h>
#include <windows.h>
#include "win32/types.h"

#else

#include "tchar_filler.h"

#endif

// "sys" includes
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

// Other system includes
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

// Unix includes
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_UTIME_H
#include <utime.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

// C++ generic includes
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>

// C generic includes
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#include <stdarg.h>
#include <assert.h>

// OpenSSL includes
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// Argtable includes
#include <argtable2.h>

// We need to manually handle some of the T functions for unicode/ansi modes
#if _UNICODE

#define _tmain wmain
typedef std::wstring TSTRING;

// Fill in the output operators
static inline std::ostream &operator<< ( std::ostream &stream, const TSTRING &str )
{
    return stream<<str.c_str();
}
#else

#define _tmain main
typedef std::string TSTRING;

#endif

#include "platform.h"

#include "autoarray.h"
#include "rcserror.h"
#if defined(__unix__) || defined(__APPLE__)
#include "autofd.h"
#include "autommap.h"
#elif defined(_WIN32)
#include "win32/autofd.h"
#include "win32/autommap.h"
#else
#error Unsupported platform
#endif

#endif // PRECOMPILED_HEADERS_H
