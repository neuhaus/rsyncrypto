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

filelistmaptype filelist;

#define DEPEND_BLOCK(type, curtype) do { if( blocks.find(type)==blocks.end() ) \
    throw rscerror("Corrupt filelist - block " #curtype " depends on block " #type); } while(false)
#define BLOCK_MINSIZE(type, size) do { if( block_length<(size) ) \
    throw rscerror("Corrupt filelist - " #type " block too short"); } while(false)
        
bool metadata::readblock( const autommap &map, size_t offset, size_t *block_size, std::set<uint16_t> &blocks )
{
    const unsigned char *block=map.get_uc()+offset;
    size_t endpos=map.getsize()-offset;

    if( endpos<4 ) {
        // Block must be at least 4 bytes long
        throw rscerror("Corrupt filelist - truncated block");
    }

    const uint16_t *usp=reinterpret_cast<const uint16_t *>(block);
    size_t block_length=ntohs(usp[0]);
    uint16_t type=ntohs(usp[1]);

    if( block_length>endpos )
        // Block tried to exceed file's length
        throw rscerror("Corrupt filelist - block file overrun");

    if( !blocks.insert(type).second ) {
        // Duplicate block type
        throw rscerror("Corrupt filelist - duplicate block type");
    }
    
    // Handle the specific block
    // XXX Think of a more generic way to do this
    switch(type)
    {
    case BLK_TYPE_PLATFORM:
        // May need to translate the path name to local directory seperator
        BLOCK_MINSIZE(BLK_TYPE_PLATFORM, 7);
        dirsep=reinterpret_cast<const char *>(block)[5];
        break;
    case BLK_TYPE_OFILENAME:
        // Make sure we have already seen the platform block
        DEPEND_BLOCK(BLK_TYPE_PLATFORM, BLK_TYPE_OFILENAME);
        BLOCK_MINSIZE(BLK_TYPE_OFILENAME, 7);
        break;
    case BLK_TYPE_EFILENAME:
        break;
    case BLK_TYPE_POSIX_PERM:
        break;
    case BLK_TYPE_NOP:
        break;
    case BLK_TYPE_EOC:
        break;
    default:
        // Unknown block type
        break;
    }

    return true;
}

size_t metadata::readchunk( const autommap &map, size_t offset, bool encrypt )
{
    size_t chunk_offset=0;
    size_t block_size=0;

    metadata data;
    std::set<uint16_t> blocks;
    
    while( offset+chunk_offset<map.getsize() &&
            data.readblock( map, offset+chunk_offset, &block_size, blocks ) )
        chunk_offset+=block_size;

    if( offset+chunk_offset>=map.getsize() )
        throw rscerror("Corrupt filelist - truncated chunk");

    // Make sure that all mandatory fields were present
    for(uint16_t i=0; i<=BLK_TYPE_MAX_MANDATORY; ++i )
        if( blocks.find(i)==blocks.end() )
            throw rscerror("Corrupt filelist - missing mandatory block");
    
    // Hashing direction (encoded->unencoded file names or vice versa) depends on whether we are encrypting or
    // decrypting
    std::string key;
    if( encrypt ) {
        key=data.plainname;
    } else {
        key=data.ciphername;
    }

    if( !filelist.insert(filelistmaptype::value_type(key, data)).second ) {
        // filelist already had an item with the same key
        throw rscerror("Corrupt filelist - duplicate key");
    }
    
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
