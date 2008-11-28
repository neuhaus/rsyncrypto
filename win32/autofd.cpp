#include <precomp.h>
#include "autofd.h"

// Translate utf-8 to wide characters
static auto_array<wchar_t> a2u( const char *str )
{
    int newlen=MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, NULL, 0 );

    if( newlen==0 ) {
        throw( rscerror( _T("Ansi to Unicode conversion error"), Error2errno(GetLastError())) );
    }

    auto_array<wchar_t> buffer(new wchar_t[newlen+1]);

    if( MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, buffer.get(), newlen )==0 ) {
        throw( rscerror( _T("Ansi to Unicode conversion error"), Error2errno(GetLastError())) );
    }

    buffer[newlen]=L'\0';

    return buffer;
}

// Unconditionally convert from Wide to ANSI
static auto_array<char> u2a( const wchar_t *str )
{
    int newlen=WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str, -1, NULL, 0, NULL, NULL );

    if( newlen==0 ) {
        throw( rscerror( _T("Unicode to Ansi conversion error"), GetLastError()) );
    }

    auto_array<char> buffer(new char[newlen+1]);

    if( WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, str, -1, buffer.get(), newlen, NULL, NULL )==0 ) {
        throw( rscerror( _T("Ansi to Unicode conversion error"), GetLastError()) );
    }

    buffer[newlen]='\0';

    return buffer;
}


FILETIME autofd::ut2ft( time_t unixtime )
{
    ULARGE_INTEGER res;
    static const ULONGLONG epoch_start=116444736000000000;

    res.QuadPart=unixtime;
    res.QuadPart*=10*1000*1000;
    res.QuadPart+=epoch_start;

    FILETIME ret;

    ret.dwHighDateTime=res.HighPart;
    ret.dwLowDateTime=res.LowPart;

    return ret;
}

time_t autofd::ft2ut( FILETIME ft )
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

autofd::autofd( const char *pathname, int flags, mode_t mode, bool utf8 ) : f_eof(false)
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

    HANDLE newfile;
    if( !utf8 )
        newfile=CreateFileA(pathname, access,
            FILE_SHARE_READ|FILE_SHARE_WRITE, // We are a unix program at heart
            NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL );
    else {
        // Need to translate the file name to unicode
        newfile=CreateFileW(a2u(pathname).get(), access,
            FILE_SHARE_READ|FILE_SHARE_WRITE, // We are a unix program at heart
            NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL );
    }
    static_cast<autohandle &>(*this)=autohandle(newfile);

    if( *this==INVALID_HANDLE_VALUE )
        throw EXCEPT_CLASS("file open failed", Error2errno(GetLastError()), pathname );
}

ssize_t autofd::read( file_t fd, void *buf, size_t count )
{
    DWORD ures;
    if( !ReadFile( fd, buf, count, &ures, NULL ) && GetLastError()!=ERROR_BROKEN_PIPE )
        throw rscerror("read failed", Error2errno(GetLastError()));

    return ures;
}

ssize_t autofd::read( void *buf, size_t count ) const
{
    ssize_t num=read( *static_cast<const autohandle *>(this), buf, count );

    if( num==0 )
        f_eof=true;

    return num;
}

ssize_t autofd::write( file_t fd, const void *buf, size_t count )
{
    DWORD written;
    if( !WriteFile( fd, buf, count, &written, NULL ) )
        throw rscerror("write failed", Error2errno(GetLastError()));

    return written;
}

struct stat autofd::stat( const char *file_name )
{
    struct stat ret;
    WIN32_FILE_ATTRIBUTE_DATA data;
    if( !GetFileAttributesEx( file_name, GetFileExInfoStandard, &data ) )
        throw rscerror("stat failed", Error2errno(GetLastError()), file_name);

    ZeroMemory( &ret, sizeof(ret) );
    ret.st_atime=ft2ut(data.ftLastAccessTime);
    ret.st_ctime=ft2ut(data.ftCreationTime);
    ret.st_mtime=ft2ut(data.ftLastWriteTime);
    ret.st_dev=0;
    ret.st_rdev=0;

    if( data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT ) {
        // The Vista equivalent of a symbolic link, more or less
        ret.st_mode=S_IFLNK;
    } else if( data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY ) {
        ret.st_mode=S_IFDIR;
    } else {
        ret.st_mode=S_IFREG;
    }

    ret.st_nlink=1;
    ret.st_size=data.nFileSizeHigh;
    ret.st_size<<=32;
    ret.st_size|=data.nFileSizeLow;

    return ret;
}

struct stat autofd::fstat() const
{
    struct stat ret;
    BY_HANDLE_FILE_INFORMATION info;

    if( !GetFileInformationByHandle(*this, &info ) )
        throw rscerror("stat failed", Error2errno(GetLastError()));

    ZeroMemory(&ret, sizeof(ret));
    ret.st_atime=ft2ut(info.ftLastAccessTime);
    ret.st_ctime=ft2ut(info.ftCreationTime);
    ret.st_dev=0; // For a device - handle. Otherwise 0
    ret.st_mode=S_IFREG; // unix mode
    ret.st_mtime=ft2ut(info.ftLastWriteTime);
    ret.st_nlink=static_cast<short>(info.nNumberOfLinks); // nlink
    ret.st_rdev=0; // same as dev
    ret.st_size=info.nFileSizeHigh;
    ret.st_size<<=32;
    ret.st_size|=info.nFileSizeLow;

    return ret;
}

off_t autofd::lseek( file_t file, off_t offset, int whence )
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
    throw rscerror("Invalid whence given", EINVAL);
    }

    LONG offsethigh, offsetlow;
    offsetlow=static_cast<LONG>(offset);
    offsethigh=static_cast<LONG>(offset>>32);
    offsetlow=SetFilePointer( file, offsetlow, &offsethigh, dwMoveMethod );

    offset=offsethigh;
    offset<<=32;
    offset|=offsetlow;

    return offset;
}

int autofd::utimes( const char *filename, const struct timeval tv[2])
{
    FILETIME modtime, accesstime;

    accesstime=ut2ft(tv[0].tv_sec);
    modtime=ut2ft(tv[1].tv_sec);

    // The only function in Windows that sets file modification/access times does so for
    // open files only, so we have no choice but to open the file for write access
    autofd file(filename, O_WRONLY);
    if( SetFileTime(file, NULL, &accesstime, &modtime ) )
        return 0;
    else {
        errno=Error2errno(GetLastError());
        return -1;
    }
}

autofd autofd::dup( int filedes )
{
    autofd orig_std(GetStdHandle(filedes));
    autofd ret(orig_std.Duplicate(false));
    orig_std.release();
    return ret;
}

void autofd::rmdir( const char *pathname )
{
    if( !RemoveDirectory(pathname) ) {
        DWORD error=GetLastError();

        if( error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND )
            throw rscerror("Error removing directory", Error2errno(error), pathname);
    }
}

std::string autofd::readline() const
{
    std::string ret;
    char ch;

    while( read( &ch, 1 )==1 && ch!='\n' ) {
        ret+=ch;
    }

    if( ch=='\n' && ret.length()>0 && ret[ret.length()-1]=='\r' )
        ret.resize(ret.length()-1);

    return ret;
}
