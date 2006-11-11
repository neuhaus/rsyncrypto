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

typedef int file_t;

// autofd handles file descriptors
class autofd {
    file_t fd;
    mutable bool owner, f_eof;

    file_t release() const
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
    explicit autofd( file_t fd_p ) : fd(fd_p), owner(valid()), f_eof(false)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( file_t fd_p, bool except ) : fd(fd_p), owner(valid()), f_eof(false)
    {
        if( except && valid() )
            throw EXCEPT_CLASS("file open failed", errno);
    }

    autofd( const char *pathname, int flags, mode_t mode=0 ) : owner(true), f_eof(false)
    {
        fd=open( pathname, flags, mode );

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
    file_t get() const
    {
        return fd;
    }
    operator file_t() const
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
    bool valid() const
    {
        return fd!=-1;
    }

    // Standard io operations
private:
    static ssize_t read( file_t fd, void *buf, size_t count )
    {
        ssize_t res=::read( fd, buf, count );

        if( res==-1 )
            throw rscerror("read failed", errno);

        return res;
    }
public:
    ssize_t read( void *buf, size_t count ) const
    {
        ssize_t num=read( fd, buf, count );

        if( num==0 )
            f_eof=true;

        return num;
    }
private:
    static ssize_t write( file_t fd, const void *buf, size_t count )
    {
        ssize_t res=::write( fd, buf, count );

        if( res!=static_cast<ssize_t>(count) )
            throw rscerror("write failed", errno);

        return res;
    }
public:
    ssize_t write( const void *buf, size_t count )
    {
        return write( fd, buf, count );
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

        if( ::fstat( get(), &ret )!=0 )
            throw rscerror("stat failed", errno);

        return ret;
    }
    static off_t lseek( file_t file, off_t offset, int whence )
    {
        return ::lseek( file, offset, whence );
    }
    off_t lseek( off_t offset, int whence ) const
    {
        return lseek( fd, offset, whence );
    }
    static int utimes( const char *filename, const struct timeval tv[2])
    {
        return ::utimes( filename, tv );
    }
    static autofd dup( int filedes )
    {
        return autofd( dup(filedes) );
    }
    static void rmdir( const char *pathname )
    {
        if( ::rmdir( pathname )!=0 && errno!=ENOENT )
        {
            throw rscerror("Error removing directory", errno, pathname );
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
