/*
 * This file is part of rsyncrypto - rsync friendly encryption
 * Copyright (C) 2005 Shachar Shemesh for Lingnu Open Source Consulting (http://www.lignu.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

// automap will auto-release mmaped areas
class autofd {
    autohandle file;
    mutable bool f_eof;

    file_t release() const
    {
        return file.release();
    }

public:
    autofd() : file(INVALID_HANDLE_VALUE), f_eof(false)
    {
    }
    explicit autofd( file_t file_p ) : file(file_p), f_eof(false)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( file_t file_p, bool except ) : file(file_p), f_eof(false)
    {
        if( file==INVALID_HANDLE_VALUE )
            throw EXCEPT_CLASS("file open failed", errno);
    }

    autofd( const char *pathname, int flags, mode_t mode=0 ) : f_eof(false)
    {
        DWORD access=0, disposition=0;

        if( (flags&(O_CREAT|O_EXCL))!=0 )
            disposition=CREATE_NEW;
        else if( (flags&(O_CREAT|O_TRUNC))!=0 )
            disposition=CREATE_ALWAYS;
        else if( (flags&O_CREAT)!=0 )
            disposition=OPEN_ALWAYS;
        else if( (flags&O_TRUNC)!=0 )
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

        file=autohandle(CreateFile(pathname, access, FILE_SHARE_READ|FILE_SHARE_WRITE, // We are a unix program at heart
            NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL ));

        if( file==INVALID_HANDLE_VALUE )
            throw EXCEPT_CLASS("file open failed", GetLastError() );
    }
#endif
    autofd( const autofd &that ) : file(that.file.release()), f_eof(that.eof())
    {
    }
    ~autofd()
    {
        clear();
    }
    file_t get() const
    {
        return file;
    }
    operator file_t()
    {
        return get();
    }
    autofd &operator=( const autofd &that )
    {
        if( file!=that.file ) {
            clear();
            file=that.file;
            f_eof=that.eof();
        }

        return *this;
    }
    void clear()
    {
        file.clear();
    }

    // Standard io operations
    static ssize_t read( file_t fd, void *buf, size_t count )
    {
        DWORD ures;
        if( !ReadFile( fd, buf, count, &ures, NULL ) )
            throw rscerror("read failed", GetLastError());

        return ures;
    }
    ssize_t read( void *buf, size_t count ) const
    {
        ssize_t num=read( file, buf, count );

        if( num==0 )
            f_eof=true;

        return num;
    }
    static ssize_t write( file_t fd, const void *buf, size_t count )
    {
        DWORD written;
        if( !WriteFile( fd, buf, count, &written, NULL ) )
            throw rscerror("write failed", GetLastError());

        return written;
    }
    ssize_t write( const void *buf, size_t count )
    {
        return write( file, buf, count );
    }

    static struct stat stat( const char *file_name )
    {
        struct stat ret;

        if( ::stat( file_name, &ret )!=0 )
            throw rscerror("stat failed", errno, file_name );

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
    off_t lseek( off_t offset, int whence )
    {
        return lseek( file, offset, whence );
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
                if( sublen>0 && path[sublen]=='\\' && path[sublen+1]!='\\' ) {
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
            if( path[i]=='/' ) // XXX make sure we change / to \ 
                last=i;
        }

        return last;
    }

    static std::string combine_paths( const char *left, const char *right )
    {
        std::string ret(left);

        int i;
        // Trim trailing slashes
        for( i=ret.length()-1; i>0 && ret[i]=='/'; --i )
            ;

        ret.resize(++i);
        if( i>0 )
            ret+="/";

        // Trim leading slashes
        for( i=0; right[i]=='/'; ++i )
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
