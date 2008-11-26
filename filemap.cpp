/*
 * rsyncrypto - an rsync friendly encryption
 * Copyright (C) 2005-2008 Shachar Shemesh for Lingnu Open Source Consulting ltd.
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
 * The project's homepage is at http://rsyncrypto.lingnu.com/
 */

/*
 * The file format is ALMOST text-editor friendly. Not quite, though.
 * Each "line" contains a single character indicating the directory seperator
 * (/, \ etc). The encrypted file name, a space, and then the unencrypted file
 * name. Each line is terminated with a NULL.
 */

#include <precomp.h>
#include "rsyncrypto.h"
#include "filemap.h"

#include "file.h"

filemaptype namemap;
revfilemap reversemap; // Cypher->plain mapping for encryption usage

static const size_t CODED_FILE_ENTROPY=128;

static void replace_dir_sep( std::string &path, char dirsep )
{
    // Shortpath if we have nothing to do
    if( dirsep!=DIRSEP_C ) {
        for( std::string::iterator i=path.begin(); i!=path.end(); ++i )
        {
            if( *i==DIRSEP_C )
                throw rscerror("Untranslateable file name");

            if( *i==dirsep )
                *i=DIRSEP_C;
        }
    }
}

void filemap::fill_map( const char *list_filename, bool encrypt )
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

        size_t offset=0;
        while( offset<listfile.getsize() ) {
            filemap entry;
            char ch=-1;
	    
            entry.dirsep=listfile.get_uc()[offset++];

            if( entry.dirsep==' ' || entry.dirsep=='\0' ) {
                // Probably a filemap corrupted by rsyncrypto 1.07
                throw rscerror("Corrupt filemap - rsyncrypto_recover will, likely, fully restore it");
            }

            int i;
            for( i=0; i+offset<listfile.getsize() && (ch=listfile.get_uc()[offset+i])!=' ' &&
                ch!='\0'; ++i )
                ;

            if( ch!=' ' )
                throw rscerror("Corrupt filemap - no plaintext file");

            entry.ciphername=std::string(reinterpret_cast<const char *>(listfile.get_uc()+offset), i);
            offset+=i+1;

            for( i=0; i+offset<listfile.getsize() && (ch=listfile.get_uc()[offset+i])!='\0'; ++i )
                ;
            if( ch!='\0' )
                throw rscerror("Corrupt filemap - file is not NULL terminated");
            entry.plainname=std::string(reinterpret_cast<const char *>(listfile.get_uc()+offset), i);

            offset+=i+1;

            replace_dir_sep( entry.plainname, entry.dirsep );

            // Hashing direction (encoded->unencoded file names or vice versa) depends on whether we are
            // encrypting or decrypting
            std::string key;
            if( encrypt ) {
                key=entry.plainname;
            } else {
                key=entry.ciphername;
            }

            if( !namemap.insert(filemaptype::value_type(key, entry)).second ) {
                // filemap already had an item with the same key
                throw rscerror("Corrupt filemap - dupliacte key");
            }

            // If we are encrypting, we will also need the other map direction
            if( encrypt && !reversemap.insert(revfilemap::value_type(entry.ciphername, entry.plainname)).second ) {
                // Oops - two files map to the same cipher name
                throw rscerror("Corrupt filemap - dupliace encrypted name");
            }
        }
    }
}

static std::string bin2hex( const uint8_t *data, size_t length )
{
    std::string ret;

    for( unsigned int i=0; i<length; ++i ) {
	static const char convertable[]="0123456789ABCDEF";
	ret+=convertable[data[i]>>4];
	ret+=convertable[data[i]&0x0f];
    }

    return ret;
}

std::string filemap::namecat_encrypt( const char *left, const char *right, mode_t mode )
{
    switch( mode&S_IFMT ) {
    case S_IFREG:
	{
	    std::string c_name; // Crypted name of file

	    // Remove leading dirseps
	    while( *right==DIRSEP_C )
		right++;

	    // Find out whether we already have an encoding for this file
	    filemaptype::const_iterator iter=namemap.find(right);
	    if( iter==namemap.end() ) {
		int i=0;
		std::string encodedfile;

		// Make sure we have no encoded name collisions
		do {
		    // Need to create new encoding
		    uint8_t buffer[CODED_FILE_ENTROPY/8];

		    // Generate an encoded form for the file.
		    if( !RAND_bytes( buffer, CODED_FILE_ENTROPY/8 ) )
		    {
			throw rscerror("No random entropy for file name", 0, left);
		    }

		    encodedfile=bin2hex( buffer, sizeof(buffer) );
		} while( reversemap.find(encodedfile)!=reversemap.end() && // Found a unique encoding
			(++i)<5 ); // Tried too many times.

		if(i==5) {
		    throw rscerror("Failed to locate unique encoding for file");
		}

		filemap newdata;
		newdata.plainname=right;
		newdata.ciphername=c_name=encodedfile;
		newdata.dirsep=DIRSEP_C;

		namemap[right]=newdata;
		reversemap[encodedfile]=right;
	    } else {
		// We already have an encoding

		c_name=iter->second.ciphername;
	    }

	    // Calculate the name as results from the required directory nesting level
	    nest_name(c_name);

	    return autofd::combine_paths(left, c_name.c_str());
	}
	break;
    case S_IFDIR:
	return left;
	break;
    default:
	return autofd::combine_paths(left, right);
    }
}

std::string filemap::namecat_decrypt( const char *left, const char *right, mode_t mode )
{
    if( !S_ISREG(mode) )
	return autofd::combine_paths(left, right);

    while( *right==DIRSEP_C )
	++right;

    // Get just the file part of the path
    for( int skip=0; right[skip]!='\0'; ++skip ) {
	if( right[skip]==DIRSEP_C ) {
	    right+=skip+1;
	    skip=0;
	}
    }

    if( *right=='\0' || strcmp(right, FILEMAPNAME)==0 )
	return "";

    filemaptype::const_iterator iter=namemap.find(right);
    if( iter==namemap.end() )
	// Oops - we don't know how this file was called before we hashed it's name!
	throw rscerror("Filename translation not found", 0, right);

    return autofd::combine_paths(left, iter->second.plainname.c_str());
}

void filemap::nest_name( std::string &name )
{
    int nestlevel=VAL(nenest);
    std::string retval(name);

    while( nestlevel>0 ) {
	std::string partial(name.c_str(), nestlevel);
	retval=autofd::combine_paths(partial.c_str(), retval.c_str() );
	nestlevel--;
    }

    name=retval;
}

// Create the file name mapping file
void filemap::write_map( const char *map_filename )
{
    autofd file(map_filename, O_WRONLY|O_CREAT|O_TRUNC, 0600 );

    for( revfilemap::const_iterator i=reversemap.begin(); i!=reversemap.end(); ++i ) {
	const filemap *data=&namemap[i->second];

	file.write( &(data->dirsep), sizeof( data->dirsep ) );
	file.write( data->ciphername.c_str(), data->ciphername.length() );
	file.write( " ", 1 );
	file.write( data->plainname.c_str(), data->plainname.length() );
	file.write( "", 1 );
    }
}

void virt_recurse_dir_enc( const char *encdir, const char *plaindir, const char *keydir,
	RSA *rsa_key, encopfunc op, const char *dir_sig_part )
{
    // We scan the translation map around the "dir_sig_part" area
    std::string basedirname(dir_sig_part);
    filemaptype::iterator begin, end;

    {
	// First, make sure there is exactly one DIRSEP_C at the end of the string.
	std::string::size_type i;
	for( i=basedirname.length(); i>0 && basedirname[i-1]==DIRSEP_C; --i )
	    ;

	basedirname.resize(i);

	basedirname+=DIRSEP_C;
    }
    
    if( basedirname.length()==1 ) {
	// The significant part is, for all intent and purposes, empty. Scan entire map
	begin=namemap.begin();
	end=namemap.end();
    } else {
	// Find the first file belonging to our work group
	begin=namemap.lower_bound(basedirname);
	// And one past the last one
	basedirname[basedirname.length()-1]=basedirname[basedirname.length()-1]+1;
	end=namemap.lower_bound(basedirname);
    }

    filemaptype::iterator next;
    for( filemaptype::iterator i=begin; i!=end; i=next ) {
	next=i;
	++next;
	op( encdir, plaindir, keydir, i, rsa_key );
    }
}

void filemap::enc_file_delete( const char *source_dir, const char *dst_dir, const char *key_dir,
	filemaptype::iterator &item, RSA *rsa_key )
{
    struct stat status;
    const std::string &plainname=item->second.plainname, &orig_ciphername=item->second.ciphername;
    std::string ciphername=orig_ciphername;
    // Make sure we use the proper nesting on the file name. That's why plain name is a reference, but cipher name
    // is a copy
    nest_name(ciphername);

    const std::string dst_file(autofd::combine_paths( dst_dir, plainname.c_str() ) );
    const std::string src_file(autofd::combine_paths( source_dir, ciphername.c_str() ));
    const std::string key_file(autofd::combine_paths( key_dir, ciphername.c_str() ));

    try {
        status=autofd::lstat(dst_file.c_str());
    } catch( const rscerror &err ) {
        if( err.errornum()==ENOENT || err.errornum()==ENOTDIR ) {
            // Need to erase file
            
            if( VERBOSE(1) )
                std::cout<<"Delete "<<orig_ciphername<<" ("<<plainname<<")"<<std::endl;
            if( changes_log.get()!=NULL )
                (*changes_log.get())<<src_file<<std::endl;
            autofd::unlink( src_file.c_str() );
            if( EXISTS(delkey) ) {
                if( VERBOSE(1) )
                    std::cout<<"Delete key "<<orig_ciphername<<" ("<<plainname<<")"<<std::endl;
                autofd::unlink( key_file.c_str() );
                reversemap.erase( orig_ciphername );
                namemap.erase( item );
            }
        } else {
            throw;
        }
    }
}
