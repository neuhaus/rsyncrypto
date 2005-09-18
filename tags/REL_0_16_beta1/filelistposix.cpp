/*
 * rsyncrypto - an rsync friendly encryption
 * Copyright (C) 2005 Shachar Shemesh for Lingnu Open Source Consulting ltd.
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

// This file contains the platform specific implementation of filelist handling for posix.
#include "rsyncrypto.h"
#include "filelist.h"
#include "filelist_format.h"

// Write out the content of "filelist" into the specified file
void metadata::write_map( const char *list_filename )
{
    union {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
    } buffer;

    autofd file(list_filename, O_WRONLY|O_CREAT, 0777 );

    // Write the file magic number
#define WRITE32(a) do { buffer.u32=htonl(a); file.write( &buffer.u32, sizeof(buffer.u32) ); } while(false)
#define WRITE16(a) do { buffer.u16=htons(a); file.write( &buffer.u16, sizeof(buffer.u16) ); } while(false)
#define WRITE8(a)  do { buffer.u8=(a); file.write( &buffer.u8, sizeof(buffer.u8) ); } while(false)
#define WRITELENGTH(a) do { reminder=(a); length=(((reminder+3)/4)*4); reminder=length-reminder; \
	WRITE16(length); } while(false)
#define WRITEPAD while ((reminder--)>0) { WRITE8(0); }
#define WRITESTR(a) do { const char *macro_str=(a); for(int i=0; macro_str[i]!='\0'; ++i ) { \
    WRITE8(macro_str[i]); } WRITE8('\0'); } while( false )

    WRITE32(FILELIST_MAGIC_VER1);

    // XXX As a stopgap solution, use a standard platform name here
    static const char PLATFORM[]="Posix";

    for( revlistmap::const_iterator i=reversemap.begin(); i!=reversemap.end(); ++i ) {
	const metadata *data=&filelist[i->second];

	size_t length, reminder;
	
	// Write Block 0 - platform
	WRITE16(12); // Length
	WRITE16(BLK_TYPE_PLATFORM);
	WRITE8(PLATFORM_UNIX);
	WRITE8(DIRSEP_C);
	WRITESTR(PLATFORM);

	// Write Block 1 - plaintext file name
	WRITELENGTH(7+data->plainname.length());
	WRITE16(BLK_TYPE_OFILENAME);
	WRITE8((data->mode&S_IFMT)>>12);
	WRITE8(NAMEENC_UNKNOWN);
	WRITESTR(data->plainname.c_str());
	WRITEPAD;

	// Write Block 2 - encrypted file name
	WRITELENGTH(4+1+data->ciphername.length());
	WRITE16(BLK_TYPE_EFILENAME);
	WRITESTR(data->ciphername.c_str());
	WRITEPAD;

	// Write end of chunk marker
	WRITE16(4);
	WRITE16(BLK_TYPE_EOC);
    }
}

