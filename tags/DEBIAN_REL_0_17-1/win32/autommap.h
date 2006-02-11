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

// Win32 portable implementation
#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef _AUTOMMAP_H
#define _AUTOMMAP_H

#include <errno.h>

#define PROT_READ 0x1
#define PROT_WRITE 0x2

#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02

// automap will auto-release mmaped areas
class autommap {
    HANDLE mapping;
    void *ptr;
    size_t size;

    // Disable default copy constructor
    autommap( const autommap & );

    void mapfile(void *start, size_t length, int prot, int flags, file_t fd, off_t offset)
    {
        DWORD flProtect;
        DWORD dwDesiredAccess;

        switch( prot ) {
        case PROT_WRITE|PROT_READ:
            flProtect=PAGE_READWRITE;
            dwDesiredAccess=FILE_MAP_WRITE;
            break;
        case PROT_READ:
            flProtect=PAGE_READONLY;
            dwDesiredAccess=FILE_MAP_READ;
            break;
        default:
#if defined(EXCEPT_CLASS)
            throw EXCEPT_CLASS("Unsupported mmap protection mode");
#else
            return;
#endif
        }

        if( length==0 ) {
            DWORD highsize;
            length=GetFileSize( fd, &highsize );
            if( highsize!=0 )
                throw EXCEPT_CLASS("File too big to be mapped", ERROR_NOT_ENOUGH_MEMORY );
        }
        mapping=CreateFileMapping( fd, NULL, flProtect, 0, 0, NULL );
        if( mapping==NULL )
            throw EXCEPT_CLASS("CreateFileMapping failed", GetLastError() );

        ptr=MapViewOfFileEx( mapping, dwDesiredAccess, 0, offset, length, start );
        if( ptr==NULL ) {
            CloseHandle( mapping );
            mapping=NULL;
#if defined(EXCEPT_CLASS)
            throw EXCEPT_CLASS("mmap failed", errno);
#endif
        }

        size=length;
    }
public:
    autommap() : ptr(NULL), mapping(NULL), size(0)
    {
    }
    autommap(void *start, size_t length, int prot, int flags, file_t fd, off_t offset ) : 
                mapping(NULL), ptr(NULL), size(0)
    {
        mapfile(start, length, prot, flags, fd, offset);
    }
    // Map an entire file into memory
    autommap(file_t fd, int prot) : mapping(NULL), ptr(NULL), size(0)
    {
        mapfile(NULL, 0, prot, 0, fd, 0);
    }
    ~autommap()
    {
        clear();
    }
    void *get() const
    {
        return ptr;
    }
    unsigned char *get_uc() const
    {
        return static_cast<unsigned char *>(ptr);
    }
    autommap &operator=( autommap &that )
    {
        clear();
        ptr=that.ptr;
        mapping=that.mapping;
        that.ptr=NULL;
        that.mapping=NULL;

        return *this;
    }
    void clear()
    {
        if( ptr!=NULL ) {
            UnmapViewOfFile(ptr);
            ptr=NULL;
            CloseHandle(mapping);
        }
        ptr=NULL;
        mapping=NULL;
    }
    size_t getsize() const {
        return size;
    }
};

#endif // _AUTOMMAP_H
