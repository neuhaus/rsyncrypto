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
 * The project's homepage is at http://sourceforge.net/projects/rsyncrypto
 */


#include "rsyncrypto.h"
#include "file.h"
#include "autodir.h"
#include "crypto.h"

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

    if( utimes( destfilename, tv )==-1 )
	throw rscerror("Setting time failed", errno, destfilename );
}

static int calc_trim( const char *path, int trim_count )
{
    int ret=0;

    if( path[0]=='\0' )
        throw rscerror("Cannot trim empty path");

    do {
        if( (path[ret]=='/' || path[ret]=='\0') && ret!=0 && path[ret-1]!='/' )
            trim_count--;
    } while( trim_count>0 && path[ret++]!='\0' );

    if( trim_count>0 )
        throw rscerror("Not enough directories to trim");

    // Skip trailing slashes
    while( path[ret]=='/' )
        ret++;

    return ret;
}

static int recurse_dir_enc( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, int src_offset, bool op_handle_dir )
{
    int ret;

    autodir dir(src_dir);

    struct dirent *ent;
    while( (ent=dir.read())!=NULL ) {
        std::string src_filename(src_dir);
        src_filename+="/";
        src_filename+=ent->d_name;

        std::string dst_filename(dst_dir);
        dst_filename+="/";
        dst_filename+=src_filename.c_str()+src_offset;
        
        std::string key_filename(key_dir);
        key_filename+="/";
        key_filename+=src_filename.c_str()+src_offset;
        
        struct stat status;
        lstat( src_filename.c_str(), &status );
        switch( status.st_mode & S_IFMT ) {
        case S_IFREG:
            // Regular file
            op( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(), rsa_key );
            break;
        case S_IFDIR:
            // Directory
            if( strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0 ) {
                if( !op_handle_dir ) {
                    if( mkdir( dst_filename.c_str(), status.st_mode )!=0 && errno!=EEXIST )
                        throw rscerror("mkdir failed", errno, dst_filename.c_str());
                    if( mkdir( key_filename.c_str(), status.st_mode )!=0 && errno!=EEXIST )
                        throw rscerror("mkdir failed", errno, key_filename.c_str());

                    recurse_dir_enc( src_filename.c_str(), dst_dir, key_dir, rsa_key, op, src_offset,
                            op_handle_dir );
                } else {
                    recurse_dir_enc( src_filename.c_str(), dst_dir, key_dir, rsa_key, op, src_offset,
                            op_handle_dir );
                    op( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(), rsa_key );
                }
            }
            break;
        case S_IFLNK:
            // Symbolic link
            break;
        default:
            // Unhandled type
            throw rscerror("Unhandled file type");
            break;
        }
    }

    return ret;
}

static int file_delete( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key )
{
    struct stat status;

    if( lstat( dst_file, &status )!=0 ) {
        if( errno==ENOENT ) {
            // Need to erase file
            if( lstat( source_file, &status )==0  ) {
                switch( status.st_mode & S_IFMT ) {
                case S_IFDIR:
                    // Need to erase directory
                    if( options.verbosity>=2 )
                        std::cerr<<"Delete dirs "<<dst_file<<", "<< key_file<<std::endl;
                    rmdir( source_file );
                    rmdir( key_file );
                    break;
                case S_IFREG:
                case S_IFLNK:
                    if( options.verbosity>=2 )
                        std::cout<<"Delete files "<<dst_file<<", "<<key_file<<std::endl;
                    if( unlink( source_file )!=0 )
                        throw rscerror("Erasing file", errno, source_file );
                    if( unlink( key_file )!=0 && errno!=ENOENT )
                        throw rscerror("Erasing file", errno, key_file );
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

    return 0;
}

int dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op )
{
    int ret=0;
    // How many bytes of src_dir to skip when creating dirs under dst_dir
    int src_offset=calc_trim( src_dir, options.trim ); 

    // Implement standard recursive descent on src_dir
    if( mkdir( (std::string(dst_dir)+"/"+(src_dir+src_offset)).c_str(), S_IRWXU|S_IRGRP|S_IXGRP )!=0 &&
            errno!=EEXIST )
        throw rscerror("mkdir failed", errno, dst_dir);
    if( mkdir( (std::string(key_dir)+"/"+(src_dir+src_offset)).c_str(), S_IRWXU|S_IRGRP|S_IXGRP )!=0 &&
            errno!=EEXIST )
        throw rscerror("mkdir failed", errno, key_dir);

    ret=recurse_dir_enc( src_dir, dst_dir, key_dir, rsa_key, op, src_offset, false );

    if( options.del ) {
        std::string src_dst_name(src_dir, src_offset); // The name of the source string when used as dst
        std::string dst_src_name(dst_dir);
        if( dst_src_name[dst_src_name.length()-1]!='/' )
            dst_src_name+="/";
        int dst_src_offset=dst_src_name.length();
        dst_src_name+=src_dir+src_offset;
        ret=recurse_dir_enc( dst_src_name.c_str(), src_dst_name.c_str(), key_dir, rsa_key, file_delete,
                dst_src_offset, true );
    }
    return ret;
}

int file_encrypt( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key )
{
    std::auto_ptr<key> head;
    autofd headfd;
    struct stat status;

    // Read in the header, or generate a new one if can't
    {
        headfd=autofd(open( key_file, O_RDONLY ));
        if( headfd!=-1 ) {
            autommap headmap( headfd, PROT_READ );
            head=std::auto_ptr<key>(key::read_key( static_cast<unsigned char *>(headmap.get()) ));

            if( options.fr && ( head->get_sum_span()!=options.rollwin ||
                        head->get_sum_mod()!=options.rollsens ||
                        head->get_sum_min_dist()!=options.rollmin) ||
                    options.fk && head->get_key_size()!=options.keysize ) {
                headfd.clear();
            }
                
        }
        if( headfd==-1 ) {
            head=std::auto_ptr<key>(key::new_key(key::CYPHER_AES, options.keysize, options.rollwin,
                        options.rollsens, options.rollmin));
        }
    }

    int open_flags=O_RDONLY;
    if( options.archive ) {
#ifdef HAVE_NOATIME
        open_flags|=O_NOATIME;
#endif
        stat(source_file, &status);
    } else {
        status.st_mode=S_IRUSR|S_IWUSR|S_IRGRP;
    }

    autofd infd;
    if( strcmp(source_file, "-")!=0 )
        infd=autofd(open(source_file, open_flags), true);
    else
        infd=autofd(dup(STDIN_FILENO), true);
    autofd outfd(open(dst_file, O_CREAT|O_TRUNC|O_RDWR, status.st_mode), true);
    encrypt_file( head.get(), rsa_key, infd, outfd );
    if( headfd==-1 ) {
        write_header( key_file, head.get() );
    }

    // Set the times on the encrypted file to match the plaintext file
    infd.release();
    outfd.release();
    if( options.archive )
        copy_metadata( dst_file, &status );

    return 0;
}

int file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key)
{
    std::auto_ptr<key> head;
    // int infd, outfd, headfd;
    struct stat status;

    /* Decryption */
    autofd headfd(open( key_file, O_RDONLY ));
    if( headfd!=-1 ) {
        head=std::auto_ptr<key>(read_header( headfd ));
        close(headfd);
    }
    /* headfd indicates whether we need to write a new header to disk. -1 means yes. */

    autofd infd(open(src_file, O_RDONLY), true);
    fstat(infd, &status);
    autofd outfd(open(dst_file, O_CREAT|O_TRUNC|O_WRONLY, status.st_mode), true);
    head=std::auto_ptr<key>(decrypt_file( head.get(), rsa_key, infd, outfd ));
    if( headfd==-1 ) {
        write_header( key_file, head.get());
    }
    infd.release();
    outfd.release();
    copy_metadata( dst_file, &status );

    return 0;
}
