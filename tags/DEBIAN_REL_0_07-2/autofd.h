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
    mutable bool owner;

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
    autofd() : fd(-1), owner(false)
    {
    }
    explicit autofd( int fd_p ) : fd(fd_p), owner(fd_p!=-1?true:false)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( int fd_p, bool except ) : fd(fd_p), owner(true)
    {
        if( fd==-1 )
            throw EXCEPT_CLASS("file open failed", errno);
    }
#endif
    autofd( const autofd &that ) : fd(that.release()), owner(true)
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
        return read( fd, buf, count );
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
};

#endif // _AUTOFD_H
