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

    // Proxy class for assignment of temporaries
    struct autofd_ref {
        int fd;

        autofd_ref( int fd_p ) : fd(fd_p) {}
    };

    // Disable default copy constructor
    autofd( const autofd & );
public:
    autofd() : fd(-1)
    {
    }
    explicit autofd( int fd_p ) : fd(fd_p)
    {
    }
#if defined(EXCEPT_CLASS)
    autofd( int fd_p, bool except ) : fd(fd_p)
    {
        if( fd==-1 )
            throw EXCEPT_CLASS("file open failed", errno);
    }
#endif
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
    autofd &operator=( autofd &that )
    {
        if( fd!=that.fd ) {
            clear();
            fd=that.release();
        }

        return *this;
    }
    int release()
    {
        int ret=fd;
        fd=-1;

        return ret;
    }

    // autofd_ref tricks
    autofd( autofd_ref ref ) : fd(ref.fd) {}
    autofd &operator= ( const autofd_ref &ref )
    {
        if( ref.fd!=fd ) {
            clear();
            fd=ref.fd;
        }

        return *this;
    }
    operator autofd_ref()
    {
        return release();
    }

    
    void clear()
    {
        if( fd!=-1 ) {
            close( fd );
            fd=-1;
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
