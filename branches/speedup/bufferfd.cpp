/*
 * rsyncrypto - an rsync friendly encryption
 * Copyright (C) 2006 Shachar Shemesh for Lingnu Open Source Consulting ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * In addition, as a special exception, the rsyncrypto authors give permission
 * to link the code of this program with the OpenSSL library (or with modified
 * versions of OpenSSL that use the same license as OpenSSL), and distribute
 * linked combinations including the two. You must obey the GNU General Public
 * License in all respects for all of the code used other than OpenSSL. If you
 * modify this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * The project's homepage is at http://sourceforge.net/projects/rsyncrypto
 */

#include "rsyncrypto.h"
#include "bufferfd.h"

#ifndef min
static inline size_t min( size_t a, size_t b ) { return a<b?a:b; }
#endif

ssize_t read_bufferfd::buffer_copy( void *buf, size_t offset, size_t count ) const
{
    ssize_t filled=0;

    // Is there data already in the buffer?
    if( endpos-startpos>0 ) {
	size_t copysize=min(endpos-startpos, count);

	memcpy( reinterpret_cast<char *>(buf)+offset, buffer.get()+startpos, copysize );

	filled+=copysize;
	startpos+=copysize;
    }

    // We may wish to reset the position to the begining of the buffer
    if( endpos<=startpos ) {
	endpos=0;
	startpos=0;
    }

    return filled;
}

ssize_t read_bufferfd::read( void *buf, size_t count ) const
{
    size_t filled=buffer_copy( buf, 0, count );
    count-=filled;

    // if we still have anything we need to read in
    if( count>0 ) {
	ssize_t numread=autofd::read( buffer.get(), buf_size );
	if( numread>0 ) {
	    endpos+=numread;
	    filled+=buffer_copy( buf, filled, count );
	} else if( numread<0 ) {
	    // We have an error
	    return numread;
	}
    }

    return filled;
}