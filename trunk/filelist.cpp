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

#include "rsyncrypto.h"
#include "filelist.h"
#include "filelist_format.h"

std::map<std::string, metadata> filelist;

static bool readblock( const autommap &map, size_t offset, size_t *block_size, metadata &data )
{
    // XXX Read block, categorize it, and send it to handler function

    return true;
}

static size_t readchunk( const autommap &map, size_t offset, bool encrypt )
{
    size_t chunk_offset=0;
    size_t block_size=0;

    metadata data;
    
    while( offset+chunk_offset<map.getsize() &&
            readblock( map, offset+chunk_offset, &block_size, data ) )
        chunk_offset+=block_size;

    if( offset+chunk_offset>=map.getsize() )
        throw rscerror("Corrupt filelist - truncated chunk");

    // XXX add metadata to map
    
    return chunk_offset+block_size;
}

void metadata::fill_map( const char *list_filename, bool encrypt )
{
    bool nofile=false;
    autofd listfile_fd;

    try {
	autofd _listfile_fd( list_filename, O_RDONLY );
        listfile_fd=_listfile_fd;
    } catch( const rscerror &err ) {
	if( err.errornum()!=ENOENT )
	    throw;
	nofile=true;
    }

    // If the file doesn't exist, an empty map is what "initialization" is for us. Simply get out.
    if( !nofile ) {
	autommap listfile( listfile_fd, PROT_READ );

	// Check magic to make sure we are dealing with the correct file
	const uint32_t *ulp=static_cast<const uint32_t *>(listfile.get());
	if( *ulp!=ntohl(FILELIST_MAGIC_VER1) ) {
	    throw rscerror( "Invalid magic in filelist" );
	}
        
        size_t offset=sizeof(FILELIST_MAGIC_VER1);
        while( offset<listfile.getsize() ) {
            offset+=readchunk( listfile, offset, encrypt );
        }
    }
}
