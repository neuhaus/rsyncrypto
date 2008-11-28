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

    static struct stat lstat( const char *file_name )
    {
        return autofd::stat( file_name );
    }
    static struct stat stat( const char *file_name );
    
    struct stat fstat() const;

    static off_t lseek( file_t file, off_t offset, int whence );
    off_t lseek( off_t offset, int whence ) const
    {
        return lseek( *static_cast<const autohandle *>(this), offset, whence );
    }
    static int utimes( const char *filename, const struct timeval tv[2]);
    
    static autofd dup( int filedes );
    static void rmdir( const char *pathname );

    // Nonstandard file io
 
    // Read from the stream up to, including, the newline
    std::string readline() const;

    static void mv( const char *src, const char *dst ) {
        if( !MoveFileEx( src, dst, MOVEFILE_REPLACE_EXISTING) ) {
            throw rscerror("rename failed", Error2errno(GetLastError()), dst );
        }
    }
    static int unlink(const char *pathname)
    {
        DWORD error=ERROR_SUCCESS;
        if( !DeleteFile( pathname ) && (error=GetLastError())!=ERROR_FILE_NOT_FOUND )
            throw rscerror("Erasing file", Error2errno(GetLastError()), pathname );

        if( error!=ERROR_SUCCESS ) {
            errno=Error2errno(error);
            
            return -1;
        }

        return 0;
    }

    // Recursively create directories
    // mode is the permissions of the end directory
    // int_mode is the permissions of all intermediately created dirs
private:
    static void mkpath_actual(const std::string &path, mode_t mode)
    {
        if( !CreateDirectory( path.c_str(), NULL ) && GetLastError()!=ERROR_ALREADY_EXISTS ) {
            // "Creating" a drive letter may fail for a whole host of reasons while actually succeeding
            if( path.length()!=2 || path[1]!=':' ||
                // Got this far in the "if" only if we tried to create something of the form C:
                // Only a ERROR_INVALID_DRIVE actually means an error
                GetLastError()==ERROR_INVALID_DRIVE )

                throw rscerror("mkdir failed", Error2errno(GetLastError()), path.c_str() );
        }
    }
public:
    static void mkpath(const char *path, mode_t mode)
    {
        if( path[0]!='\0' ) {
            for( int sublen=0; path[sublen]!='\0'; sublen++ ) {
                if( sublen>0 && path[sublen]==DIRSEP_C && path[sublen+1]!=DIRSEP_C ) {
                    std::string subpath(path, sublen);
                    mkpath_actual(subpath, mode);
                }
            }

            mkpath_actual(path, mode);
        }
    }

    // Return the dir part of the name
    static int dirpart( const char *path )
    {
        int i, last=0;

        for( i=0; path[i]!='\0'; ++i ) {
            if( path[i]==DIRSEP_C )
                last=i;
        }

        return last;
    }

    static std::string combine_paths( const char *left, const char *right )
    {
        std::string ret(left);

        int i;
        // Trim trailing slashes
        for( i=ret.length()-1; i>0 && ret[i]==DIRSEP_C; --i )
            ;

        ret.resize(++i);
        if( i>0 )
            ret+=DIRSEP_S;

        // Trim leading slashes
        for( i=0; right[i]==DIRSEP_C; ++i )
            ;
        ret+=right+i;

        return ret;
    }
    // Return the length of the absolute specifier of the path
    static size_t is_absolute( const char *path ) {
        return path[0]=='\\' || path[0]!='\0' && path[1]==':';
    }
    
    // Status queries
    bool eof() const
    {
        return f_eof;
    }
};

#endif // _AUTOFD_H
