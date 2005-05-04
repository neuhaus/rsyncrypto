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

#include <unistd.h>

// automap will auto-release mmaped areas
class autofd {
    int fd;
    mutable bool owner, f_eof;

    int release() const
    {
#if defined(EXCEPT_CLASS)
        if( !owner )
            throw EXCEPT_CLASS("Releasing non-owner fd");
#endif

        owner=false;

        return fd;
    }

public:
    autofd() : fd(-1), owner(false), f_eof(false)
    {
    }
    explicit autofd( int fd_p ) : fd(fd_p), owner(fd_p!=-1?true:false), f_eof(false)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( int fd_p, bool except ) : fd(fd_p), owner(true), f_eof(false)
    {
        if( fd==-1 )
            throw EXCEPT_CLASS("file open failed", errno);
    }
#endif
    autofd( const autofd &that ) : fd(that.release()), owner(true), f_eof(false)
    {
    }
    ~autofd()
    {
        clear();
    }
    int get() const
    {
        return fd;
    }
    operator int()
    {
        return get();
    }
    autofd &operator=( const autofd &that )
    {
        if( fd!=that.fd ) {
            clear();
            fd=that.release();
            owner=true;
            f_eof=that.f_eof;
        }

        return *this;
    }
    void clear()
    {
        if( owner ) {
            close( fd );
            fd=-1;
            owner=false;
        }
    }

    // Standard io operations
    static ssize_t read( int fd, void *buf, size_t count )
    {
        ssize_t res=::read( fd, buf, count );

        if( res==-1 )
            throw rscerror("read failed", errno);

        return res;
    }
    ssize_t read( void *buf, size_t count ) const
    {
        ssize_t num=read( fd, buf, count );

        if( num==0 )
            f_eof=true;

        return num;
    }
    static void write( int fd, void *buf, size_t count )
    {
        ssize_t res=::write( fd, buf, count );

        if( res!=static_cast<ssize_t>(count) )
            throw rscerror("write failed", errno);
    }
    void write( void *buf, size_t count )
    {
        write( fd, buf, count );
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
                if( sublen>0 && path[sublen]=='/' && path[sublen+1]!='/' ) {
                    std::string subpath(path, sublen);
                    if( mkdir( subpath.c_str(), mode )!=0 && errno!=EEXIST )
                        throw rscerror("mkdir failed", errno, subpath.c_str() );
                }
            }

            if( mkdir( path, mode )!=0 && errno!=EEXIST )
                throw rscerror("mkdir failed", errno, path );
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
