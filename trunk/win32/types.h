#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef WIN32_TYPES_H
#define WIN32_TYPES_H

int Error2errno( DWORD Error );

typedef unsigned char uint8_t;
typedef UINT32 uint32_t;
typedef unsigned short uint16_t;

#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)

static inline void bzero( void *dest, size_t count )
{
    memset( dest, 0, count);
}

#endif // WIN32_TYPES_H