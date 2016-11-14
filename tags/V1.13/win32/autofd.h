// This is the Win32 version of this struct
#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef _AUTOFD_H
#define _AUTOFD_H

#include "autohandle.h"
#include "errno.h"

// Fill in missing declarations
#define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)

#define S_IFLNK 0120000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define STDIN_FILENO (STD_INPUT_HANDLE)
#define STDOUT_FILENO (STD_OUTPUT_HANDLE)
#define STDERR_FILENO (STD_ERROR_HANDLE)

// automap will auto-release mmaped areas
class autofd : public autohandle {
    mutable bool f_eof;

    static FILETIME ut2ft( time_t unixtime );
    static time_t ft2ut( FILETIME ft );
public:
    autofd() : autohandle(INVALID_HANDLE_VALUE), f_eof(false)
    {
    }
    explicit autofd( file_t file_p ) : autohandle(file_p), f_eof(false)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( file_t file_p, bool except ) : autohandle(file_p), f_eof(false)
    {
        if( *this==INVALID_HANDLE_VALUE )
            throw EXCEPT_CLASS("file handle open failed", GetLastError());
    }

    autofd( const char *pathname, int flags, mode_t mode=0, bool utf8=false );
#endif
    // Default copy constructor and operator= do exactly what we want.
    //autofd( const autofd &that )
    //autofd &operator=( const autofd &that )
    ~autofd()
    {
        clear();
    }
    file_t get() const
    {
        return *static_cast<const autohandle *>(this);
    }
    operator file_t() const
    {
        return get();
    }

    // Standard io operations
private:
    static ssize_t read( file_t fd, void *buf, size_t count );
public:
    ssize_t read( void *buf, size_t count ) const;
private:
    static ssize_t write( file_t fd, const void *buf, size_t count );
public:
    ssize_t write( const void *buf, size_t count )
    {
        return write( *static_cast<const autohandle *>(this), buf, count );
    }

    static struct stat lstat( const char *file_name, bool utf8=false )
    {
        return autofd::stat( file_name, utf8 );
    }
    static struct stat stat( const char *file_name, bool utf8=false );
    
    struct stat fstat() const;

    static off_t lseek( file_t file, off_t offset, int whence );
    off_t lseek( off_t offset, int whence ) const
    {
        return lseek( *static_cast<const autohandle *>(this), offset, whence );
    }
    static int utimes( const char *filename, const struct timeval tv[2], bool utf8=false);
    
    static autofd dup( int filedes );
    static void rmdir( const char *pathname, bool utf8=false );

    // Nonstandard file io
 
    // Read from the stream up to, including, the newline
    std::string readline() const;

    static void mv( const char *src, const char *dst, bool utf8=false );
    static int unlink(const char *pathname, bool utf8=false);

    // Recursively create directories
    // mode is the permissions of the end directory
    // int_mode is the permissions of all intermediately created dirs
    static void mkpath(const char *path, mode_t mode, bool utf8=false);

    // Return the dir part of the name
    static int dirpart( const char *path, bool utf8=false );

    static std::string combine_paths( const char *left, const char *right, bool utf8=false );

    // Return the length of the absolute specifier of the path
    static size_t is_absolute( const char *path, bool utf8=false ) {
        return path[0]=='\\' || path[0]!='\0' && path[1]==':';
    }
    
    // Status queries
    bool eof() const
    {
        return f_eof;
    }
};

#endif // _AUTOFD_H
