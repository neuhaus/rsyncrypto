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

#ifndef _AUTODIR_H
#define _AUTODIR_H

// automap will auto-release mmaped areas
class autodir {
    DIR *dir;

    // Disable default copy constructor
    autodir( const autodir & );
    autodir &operator=( const autodir & );
public:
    explicit autodir( DIR *init ) : dir(init)
    {
#if defined(EXCEPT_CLASS)
        if( dir==NULL )
            throw rscerror(errno);
#endif
    }
    ~autodir()
    {
        clear();
    }
    DIR *get() const
    {
        return dir;
    }

    void clear()
    {
        if( dir!=NULL ) {
            closedir( dir );
            dir=NULL;
        }
    }

    struct dirent *read()
    {
        return ::readdir(dir);
    }
    void rewind()
    {
        ::rewinddir(dir);
    }
    void seek( off_t offset )
    {
        seekdir( dir, offset );
    }
    off_t telldir()
    {
        return ::telldir( dir );
    }
};

#endif // _AUTODIR_H
