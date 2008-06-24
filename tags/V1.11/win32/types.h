#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef WIN32_TYPES_H
#define WIN32_TYPES_H

typedef unsigned char uint8_t;
typedef UINT32 uint32_t;
typedef unsigned short uint16_t;

typedef __int64 _off_t;
typedef _off_t off_t;
#define _OFF_T_DEFINED

typedef int ssize_t;
typedef unsigned short mode_t;
typedef HANDLE file_t;

#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)

static inline void bzero( void *dest, size_t count )
{
    memset( dest, 0, count);
}

int Error2errno( DWORD Error );

#endif // WIN32_TYPES_H