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

#ifndef _AUTOMMAP_H
#define _AUTOMMAP_H

#include <sys/mman.h>

// automap will auto-release mmaped areas
class autommap {
    void *ptr;
    size_t size;

    // Disable default copy constructor
    autommap( const autommap & );
public:
    autommap() : ptr(NULL), size(0)
    {
    }
    autommap(void *start, size_t length, int prot, int flags, int fd, off_t offset ) : 
                ptr(mmap(start, length, prot, flags, fd, offset)), size(length)
    {
        if( ptr==NULL ) {
            size=0;
#if defined(EXCEPT_CLASS)
            throw EXCEPT_CLASS(errno);
#endif
        }
    }
    // Map an entire file into memory
    autommap(int fd, int prot) : ptr(NULL), size(0)
    {
        struct stat filestat;
        if( fstat(fd, &filestat)==0 ) {
            autommap that(NULL, filestat.st_size, prot, MAP_SHARED, fd, 0);
            *this=that;
        }
#if defined(EXCEPT_CLASS)
        else
            throw EXCEPT_CLASS(errno);
#endif
    }
    ~autommap()
    {
        release();
    }
    void *get() const
    {
        return ptr;
    }
    autommap &operator=( autommap &that )
    {
        release();
        ptr=that.ptr;
        size=that.size;
        that.ptr=NULL;
        that.size=0;

        return *this;
    }
    void release()
    {
        if( ptr!=NULL ) {
            munmap( ptr, size );
        }
        ptr=NULL;
        size=0;
    }
};

#endif // _AUTOMMAP_H
