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
            throw EXCEPT_CLASS(errno);
    }
#endif
    ~autofd()
    {
        close(fd);
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
        release();
        fd=that.fd;
        that.fd=-1;

        return *this;
    }
    void release()
    {
        if( fd!=-1 ) {
            close( fd );
        }
        fd=-1;
    }
};

#endif // _AUTOFD_H
