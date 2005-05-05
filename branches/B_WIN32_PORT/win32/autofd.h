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

#ifndef _AUTOFD_H
#define _AUTOFD_H

typedef int ssize_t;
typedef unsigned short mode_t;
typedef HANDLE file_t;

// automap will auto-release mmaped areas
class autofd {
    file_t file;
    mutable bool owner, f_eof;

    file_t release() const
    {
#if defined(EXCEPT_CLASS)
        if( !owner )
            throw EXCEPT_CLASS("Releasing non-owner fd");
#endif

        owner=false;

        return file;
    }

public:
    autofd() : file(INVALID_HANDLE_VALUE), owner(false), f_eof(false)
    {
    }
    explicit autofd( file_t file_p ) : file(file_p), owner(file_p!=INVALID_HANDLE_VALUE?true:false),
        f_eof(false)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( file_t file_p, bool except ) : file(file_p), owner(true), f_eof(false)
    {
        if( file==INVALID_HANDLE_VALUE )
            throw EXCEPT_CLASS("file open failed", errno);
    }
#endif
    autofd( const autofd &that ) : file(that.release()), owner(true), f_eof(false)
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
            file=that.release();
            owner=true;
            f_eof=that.f_eof;
        }

        return *this;
    }
    void clear()
    {
        if( owner ) {
            CloseHandle( file );
            file=INVALID_HANDLE_VALUE;
            owner=false;
        }
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
    static void write( file_t fd, void *buf, size_t count )
    {
        DWORD written;
        if( !WriteFile( fd, buf, count, &written, NULL ) )
            throw rscerror("write failed", GetLastError());
    }
    void write( void *buf, size_t count )
    {
        write( file, buf, count );
    }

    static struct stat stat( const char *file_name )
    {
        struct stat ret;

        if( ::stat( file_name, &ret )!=0 )
            throw rscerror("stat failed", errno, file_name );

        return ret;
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
            if( path[i]=='/' )
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
