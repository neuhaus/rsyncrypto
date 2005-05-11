// This is the Win32 version of this struct
#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef _AUTOFD_H
#define _AUTOFD_H

#include "autohandle.h"

typedef int ssize_t;
typedef unsigned short mode_t;
typedef HANDLE file_t;

// Fill in missing declarations
#define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)

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

    static time_t ft2ut( FILETIME ft )
    {
        // Converts FILETIME to time_t
        static const ULONGLONG epoch_start=116444736000000000;
        ULARGE_INTEGER unified_ft;
        unified_ft.LowPart=ft.dwLowDateTime;
        unified_ft.HighPart=ft.dwHighDateTime;
        if( unified_ft.QuadPart<epoch_start )
            return -1;

        unified_ft.QuadPart-=epoch_start;
        unified_ft.QuadPart/=10*1000*1000;
        if( unified_ft.HighPart!=0 )
            return -1;

        return unified_ft.LowPart;
    }
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
            throw EXCEPT_CLASS("file open failed", GetLastError());
    }

    autofd( const char *pathname, int flags, mode_t mode=0 ) : f_eof(false)
    {
        DWORD access=0, disposition=0;

#define CHECK_MASK(a) ((flags&(a))==(a))
        if( CHECK_MASK(O_CREAT|O_EXCL) )
            disposition=CREATE_NEW;
        else if( CHECK_MASK(O_CREAT|O_TRUNC) )
            disposition=CREATE_ALWAYS;
        else if( CHECK_MASK(O_CREAT) )
            disposition=OPEN_ALWAYS;
        else if( CHECK_MASK(O_TRUNC) )
            disposition=TRUNCATE_EXISTING;
        else
            disposition=OPEN_EXISTING;

        switch( flags&O_ACCMODE ) {
        case O_RDONLY:
            access=GENERIC_READ;
            break;
        case O_WRONLY:
            access=GENERIC_WRITE;
            break;
        case O_RDWR:
            access=GENERIC_READ|GENERIC_WRITE;
            break;
        }

        ODS("CreateFile %s\n", pathname);
        static_cast<autohandle &>(*this)=autohandle(CreateFile(pathname, access,
            FILE_SHARE_READ|FILE_SHARE_WRITE, // We are a unix program at heart
            NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL ));

        if( *this==INVALID_HANDLE_VALUE )
            throw EXCEPT_CLASS("file open failed", GetLastError() );
    }
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
    static ssize_t read( file_t fd, void *buf, size_t count )
    {
        DWORD ures;
        if( !ReadFile( fd, buf, count, &ures, NULL ) )
            throw rscerror("read failed", GetLastError());

        ODS("Read %d bytes from %08x\n", ures, fd);
        return ures;
    }
    ssize_t read( void *buf, size_t count ) const
    {
        ssize_t num=read( *static_cast<const autohandle *>(this), buf, count );

        if( num==0 )
            f_eof=true;

        return num;
    }
    static ssize_t write( file_t fd, const void *buf, size_t count )
    {
        DWORD written;
        ODS("Going to write %d bytes to %08x\n", count, fd );
        if( !WriteFile( fd, buf, count, &written, NULL ) )
            throw rscerror("write failed", GetLastError());

        ODS("Wrote %d bytes to %08x\n", written, fd);
        return written;
    }
    ssize_t write( const void *buf, size_t count )
    {
        return write( *static_cast<const autohandle *>(this), buf, count );
    }

    static struct stat stat( const char *file_name )
    {
        struct stat ret;

        if( ::stat( file_name, &ret )!=0 )
            throw rscerror("stat failed", errno, file_name );

        return ret;
    }
    struct stat fstat() const
    {
        struct stat ret;
        BY_HANDLE_FILE_INFORMATION info;

        if( !GetFileInformationByHandle(*this, &info ) )
            throw rscerror("stat failed", GetLastError());

        ZeroMemory(&ret, sizeof(ret));
        ret.st_atime=ft2ut(info.ftLastAccessTime);
        ret.st_ctime=ft2ut(info.ftCreationTime);
        ret.st_dev=0; // For a device - handle. Otherwise 0
        //ret.st_mode; // unix mode
        ret.st_mtime=ft2ut(info.ftLastWriteTime);
        ret.st_nlink=info.nNumberOfLinks; // nlink
        ret.st_rdev=0; // same as dev
        ret.st_size=info.nFileSizeLow;

        return ret;
    }

    static off_t lseek( file_t file, off_t offset, int whence )
    {
        DWORD dwMoveMethod=0;

        switch( whence ) {
        case SEEK_SET:
            dwMoveMethod=FILE_BEGIN;
            break;
        case SEEK_CUR:
            dwMoveMethod=FILE_CURRENT;
            break;
        case SEEK_END:
            dwMoveMethod=FILE_END;
            break;
        default:
            throw rscerror("Invalid whence given");
        }

        return SetFilePointer( file, offset, NULL, dwMoveMethod );
    }
    off_t lseek( off_t offset, int whence ) const
    {
        return lseek( *static_cast<const autohandle *>(this), offset, whence );
    }
    static int utimes( const char *filename, const struct timeval tv[2])
    {
        struct utimbuf buf;
        buf.actime=tv[0].tv_sec;
        buf.modtime=tv[1].tv_sec;
        return utime(filename, &buf);
    }
    static autofd dup( int filedes )
    {
        autofd orig_std(GetStdHandle(filedes));
        autofd ret(orig_std.Duplicate(false));
        orig_std.release();
        return ret;
    }
    static void rmdir( const char *pathname )
    {
        if( !RemoveDirectory(pathname) ) {
            DWORD error=GetLastError();

            if( error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND )
                throw rscerror("Error removing directory", error, pathname);
        }
    }
    // Nonstandard file io
 
    // Read from the stream up to, including, the newline
    std::string readline() const
    {
        std::string ret;
        char ch;

        while( read( &ch, 1 )==1 && ch!='\n' ) {
            ret+=ch;
        }

        return ret;
    }
    // Recursively create directories
    // mode is the permissions of the end directory
    // int_mode is the permissions of all intermediately created dirs
    static void mkpath(const char *path, mode_t mode)
    {
        if( path[0]!='\0' ) {
            for( int sublen=0; path[sublen]!='\0'; sublen++ ) {
                if( sublen>0 && path[sublen]==DIRSEP_C && path[sublen+1]!=DIRSEP_C ) {
                    std::string subpath(path, sublen);
                    if( !CreateDirectory( subpath.c_str(), NULL ) &&
                        GetLastError()!=ERROR_FILE_EXISTS )
                        throw rscerror("mkdir failed", GetLastError(), subpath.c_str() );
                }
            }

            if( CreateDirectory( path, NULL )!=0 && GetLastError()!=ERROR_FILE_EXISTS )
                throw rscerror("mkdir failed", GetLastError(), path );
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
    // Status queries
    bool eof() const
    {
        return f_eof;
    }
};

#endif // _AUTOFD_H
