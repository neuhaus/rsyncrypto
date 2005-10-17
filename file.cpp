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
#include "file.h"
#include "autodir.h"
#include "crypto.h"
#include "filemap.h"

static void copy_metadata( const char *destfilename, const struct stat *data )
{
    struct timeval tv[2];

    tv[0].tv_sec=data->st_atime;
    tv[1].tv_sec=data->st_mtime;
#if HAVE_STAT_NSEC
    tv[0].tv_usec=data->st_atime_nsec/1000;
    tv[1].tv_usec=data->st_mtime_nsec/1000;
#else
    tv[0].tv_usec=0;
    tv[1].tv_usec=0;
#endif

    if( autofd::utimes( destfilename, tv )==-1 )
	throw rscerror("Setting time failed", errno, destfilename );
}

int calc_trim( const char *path, int trim_count )
{
    int ret=0;

    if( path[0]=='\0' )
        throw rscerror("Cannot trim empty path");

    if( trim_count==0 )
	return 0;

    do {
        if( (path[ret]==DIRSEP_C || path[ret]=='\0') && ret!=0 && path[ret-1]!=DIRSEP_C )
            trim_count--;
    } while( trim_count>0 && path[ret++]!='\0' );

    if( trim_count>0 )
        throw rscerror("Not enough directories to trim");

    // Skip trailing slashes
    while( path[ret]==DIRSEP_C )
        ret++;

    return ret;
}

void filelist_encrypt( const char *src, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname, namefunc srcnameop, namefunc dstnameop, namefunc keynameop )
{
    autofd srcfd;

    if( strcmp(src, "-")==0 ) {
        // Src is stdin
        srcfd=autofd::dup(STDIN_FILENO);
    } else {
        srcfd=autofd(src, O_RDONLY);
    }

    while( !srcfd.eof() ) {
        std::string srcname=srcfd.readline();

        if( srcname!="" ) try {
	    // Seperate the prefix from the actual name
	    size_t src_offset=calc_trim( srcname.c_str(), VAL(trim) );
	    std::string src_prefix(srcname.c_str(), src_offset);
	    srcname=std::string(srcname.c_str()+src_offset);
	    
	    // Perform name (de)mangling
	    std::string src=srcnameop( src_prefix.c_str(), srcname.c_str(), 0 );

            struct stat filestat=autofd::stat( src.c_str() );

            switch( filestat.st_mode&S_IFMT ) {
            case S_IFREG:
                {
                    if( VERBOSE(1) )
                        std::cerr<<opname<<" file: "<<srcname<<std::endl;

                    std::string dstfile=dstnameop( dst_dir, srcname.c_str(), filestat.st_mode );
                    std::string keyfile=keynameop( key_dir, srcname.c_str(), filestat.st_mode );

                    op( src.c_str(), dstfile.c_str(), keyfile.c_str(), rsa_key );
                }
                break;
            case S_IFDIR:
                {
                    if( VERBOSE(1) )
                        std::cerr<<opname<<" directory: "<<srcname<<std::endl;

		    // XXX What happens if there is an actual directory inside a meta-encrypted dir?
                    std::string dstfile=dstnameop( dst_dir, srcname.c_str(), filestat.st_mode );
                    std::string keyfile=keynameop( key_dir, srcname.c_str(), filestat.st_mode );

                    dir_encrypt( src.c_str(), dst_dir, key_dir, rsa_key, op, opname, dstnameop, keynameop );
                }
                break;
            default:
                throw rscerror("Unsupported file type");
                break;
            }
        } catch( const rscerror &err ) {
            std::cerr<<"Error in encryption of "<<srcname<<": "<<err.error()<<std::endl;
        }
    }
}

static void recurse_dir_enc( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, int src_offset, bool op_handle_dir, const char *opname, namefunc dstname,
	namefunc keyname )
{
    autodir dir(src_dir);

    struct dirent *ent;
    while( (ent=dir.read())!=NULL ) {
        std::string src_filename(autofd::combine_paths(src_dir, ent->d_name));
        
        struct stat status, dststat;
        lstat( src_filename.c_str(), &status );
        std::string dst_filename(dstname(dst_dir, src_filename.c_str()+src_offset, status.st_mode));
        std::string key_filename(keyname(key_dir, src_filename.c_str()+src_offset, status.st_mode));

	if( dst_filename.length()>0 ) {
	    switch( status.st_mode & S_IFMT ) {
	    case S_IFREG:
		// Regular file
		{
		    if( !EXISTS(changed) || lstat( dst_filename.c_str(), &dststat )!=0 ||
			    dststat.st_mtime!=status.st_mtime ) {
			if( VERBOSE(1) && opname!=NULL )
			    std::cerr<<opname<<" "<<src_filename<<std::endl;
			try {
			    op( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(), rsa_key );
			} catch( const rscerror &err ) {
			    std::cerr<<opname<<" "<<dst_filename<<" error: "<<err.error()<<std::endl;
			}
		    } else if( VERBOSE(2) && opname!=NULL ) {
			std::cerr<<"Skipping unchanged file "<<src_filename<<std::endl;
		    }
		}
		break;
	    case S_IFDIR:
		// Directory
		if( strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0 ) {
		    if( !op_handle_dir ) {
			recurse_dir_enc( src_filename.c_str(), dst_dir, key_dir, rsa_key, op, src_offset,
				op_handle_dir, opname, dstname, keyname );
		    } else {
			recurse_dir_enc( src_filename.c_str(), dst_dir, key_dir, rsa_key, op, src_offset,
				op_handle_dir, opname, dstname, keyname );
			op( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(), rsa_key );
		    }
		}
		break;
#if defined S_IFLNK
	    case S_IFLNK:
		// Symbolic link
		break;
#endif
	    default:
		// Unhandled type
		throw rscerror("Unhandled file type");
		break;
	    }
	}
    }
}

static void file_delete( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key )
{
    struct stat status;

    if( lstat( dst_file, &status )!=0 ) {
	if( errno==ENOENT ) {
	    // Need to erase file
	    if( lstat( source_file, &status )==0  ) {
		switch( status.st_mode & S_IFMT ) {
		case S_IFDIR:
		    // Need to erase directory
		    if( VERBOSE(1) )
			std::cerr<<"Delete dirs "<<dst_file<<", "<< key_file<<std::endl;
		    autofd::rmdir( source_file );
		    autofd::rmdir( key_file );
		    break;
		case S_IFREG:
#if defined S_IFLNK
		case S_IFLNK:
#endif
		    if( VERBOSE(1) )
			std::cout<<"Delete "<<source_file<<std::endl;
		    if( unlink( source_file )!=0 )
			throw rscerror("Erasing file", errno, source_file );
		    if( EXISTS(delkey) ) {
			if( VERBOSE(1) )
			    std::cout<<"Delete "<<key_file<<std::endl;
			if( unlink( key_file )!=0 && errno!=ENOENT )
			    throw rscerror("Erasing file", errno, key_file );
		    }
                    break;
                default:
                    throw rscerror("Unhandled file type", 0, source_file );
                }
            } else if( errno!=ENOENT )
                throw rscerror("Can't stat file to delete", errno, source_file );
            else
                throw rscerror("Internal error", errno, source_file );
        } else
            throw rscerror("Stat failed", errno, dst_file);
    }
}

void dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname, namefunc dstname, namefunc keyname )
{
    // How many bytes of src_dir to skip when creating dirs under dst_dir
    int src_offset=calc_trim( src_dir, VAL(trim) ); 

    // Implement standard recursive descent on src_dir
    autofd::mkpath( dstname(dst_dir, src_dir+src_offset, S_IFDIR ).c_str(), 0777 );
    autofd::mkpath( keyname(key_dir, src_dir+src_offset, S_IFDIR ).c_str(), 0700 );

    recurse_dir_enc( src_dir, dst_dir, key_dir, rsa_key, op, src_offset, false, opname, dstname, keyname );

    if( EXISTS(del) ) {
        std::string src_dst_name(src_dir, src_offset); // The name of the source string when used as dst
        std::string dst_src_name(dst_dir);
        int dst_src_offset=dst_src_name.length();
        dst_src_name=autofd::combine_paths(dst_src_name.c_str(), src_dir+src_offset);
        
        recurse_dir_enc( dst_src_name.c_str(), src_dst_name.c_str(), key_dir, rsa_key, file_delete,
                dst_src_offset, true, NULL, dstname, keyname );
    }
}

void file_encrypt( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key )
{
    std::auto_ptr<key> head;
    autofd headfd;

    // Read in the header, or generate a new one if can't
    {
        try {
            headfd=autofd( key_file, O_RDONLY );
        } catch( const rscerror &err ) {
            if( err.errornum()!=ENOENT )
                throw;
        }
        if( headfd.valid() ) {
            autommap headmap( headfd, PROT_READ );
            head=std::auto_ptr<key>(key::read_key( static_cast<unsigned char *>(headmap.get()) ));

            if( EXISTS(fr) && ( head->get_sum_span()!=static_cast<uint32_t>(VAL(rollwin)) ||
                        head->get_sum_mod()!=static_cast<uint32_t>(VAL(rollsens)) ||
                        head->get_sum_min_dist()!=static_cast<uint32_t>(VAL(rollmin))) ||
                    EXISTS(fk) && head->get_key_size()!=static_cast<uint32_t>(VAL(keysize)) ) {
                headfd.clear();
            }
                
        }
        if( !headfd.valid() ) {
            head=std::auto_ptr<key>(key::new_key(key::CYPHER_AES, VAL(keysize), VAL(rollwin),
                        VAL(rollsens), VAL(rollmin)));
        }
    }

    int open_flags=O_RDONLY;
#ifdef HAVE_NOATIME
    open_flags|=O_NOATIME;
#endif
    bool archive=!EXISTS(noarch);

    autofd infd;
    if( strcmp(source_file, "-")!=0 )
        infd=autofd(source_file, open_flags);
    else {
        infd=autofd::dup(STDIN_FILENO);
        // If source is stdin, there is nothing to archive
        archive=false;
    }

    autofd::mkpath( std::string(dst_file, autofd::dirpart(dst_file)).c_str(), 0777 );
    autofd outfd(dst_file, O_CREAT|O_TRUNC|O_RDWR, 0666);
    encrypt_file( head.get(), rsa_key, infd, outfd );
    if( !headfd.valid() ) {
        write_header( key_file, head.get() );
    }

    // Set the times on the encrypted file to match the plaintext file
    if( archive ) {
        struct stat status;

        stat(source_file, &status);
        copy_metadata( dst_file, &status );
    }
}

void file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key)
{
    std::auto_ptr<key> head;
    // int infd, outfd, headfd;
    struct stat status;

    /* Decryption */
    autofd headfd;
    try {
        headfd=autofd( key_file, O_RDONLY );
    } catch( const rscerror &err ) {
        if( err.errornum()!=ENOENT )
            throw;
    }
    bool headeread=headfd.valid();
    // headread indicates whether we need to write a new header to disk.
    if( headeread ) {
        head=std::auto_ptr<key>(read_header( headfd ));
        headfd.clear();
    }

    autofd infd(src_file, O_RDONLY);
    status=infd.fstat();

    autofd::mkpath( std::string(dst_file, autofd::dirpart(dst_file)).c_str(), 0777);
    autofd outfd(dst_file, O_CREAT|O_TRUNC|O_WRONLY, 0666);
    head=std::auto_ptr<key>(decrypt_file( head.get(), rsa_key, infd, outfd ));
    if( !headeread ) {
        write_header( key_file, head.get());
    }
    copy_metadata( dst_file, &status );
}

std::string name_concat( const char *left, const char *right, mode_t mode )
{
    return autofd::combine_paths( left, right );
}
