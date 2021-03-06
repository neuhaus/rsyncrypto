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

#include <precomp.h>
#include "rsyncrypto.h"
#include "file.h"
#include "autodir.h"
#include "crypto.h"
#include "filemap.h"

// Forward declaration of static functions
static void real_dir_encrypt( const char *src_dir, size_t rel, const char *dst_dir, const char *key_dir, RSA *rsa_key,
                 encryptfunc op, const char *opname, namefunc dstname, namefunc keyname );

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

    if( trim_count==0 ) {
        return 0;
    }

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
        encryptfunc op, const char *opname, prefix_func prefix_op, namefunc srcnameop, namefunc dstnameop,
        namefunc keynameop )
{
    autofd srcfd;

    if( strcmp(FILENAME(filelist), "-")==0 ) {
        // Src is stdin
        srcfd=autofd::dup(STDIN_FILENO);
    } else {
        srcfd=autofd(FILENAME(filelist), O_RDONLY);
    }

    bool error=false;

    while( !srcfd.eof() ) {
        std::string srcname=srcfd.readline();

        if( srcname!="" ) try {
            // Seperate the prefix from the actual name, and reconnect them using the prefix_op
            bool is_dir = srcname[srcname.length()-1]==DIRSEP_C;

            size_t src_offset=calc_trim( srcname.c_str(), VAL(trim) );
            // Create the string for the prefix part
            std::string src_prefix(
                prefix_op( FILENAME(src), std::string(srcname.c_str(), src_offset).c_str(),
                    autofd::is_absolute(srcname.c_str())));
            
            if( src_offset!=0 ) {
                srcname = srcname.c_str()+src_offset;
            }

            size_t trim_offset=srcname.length(); // How much has been added to the source prefix
            std::string src( srcnameop( src_prefix.c_str(), srcname.c_str(), S_IFREG ) );
            trim_offset=src.length()-trim_offset;

            int st_mode = is_dir ? S_IFDIR : S_IFREG;
            std::string dstfile( dstnameop( dst_dir, srcname.c_str(), st_mode ) );
            std::string keyfile( keynameop( key_dir, srcname.c_str(), st_mode ) );

            struct stat srcstat;
            srcstat = autofd::stat( src.c_str() );

            if( !is_dir ) {
                // File list points at a regular file
                struct stat dststat;

                bool found=true;
                try {
                    dststat=autofd::lstat( dstfile.c_str() );
                } catch( const rscerror & ) {
                    found=false;
                }
                if( !EXISTS(changed) || !found ||
                        abs(dststat.st_mtime-srcstat.st_mtime)>VAL(mod_win) )
                {
                    // Report to stderr the operation
                    if( VERBOSE(1) )
                        std::cerr<<opname<<" file: "<<srcname<<std::endl;

                    op( src.c_str(), dstfile.c_str(), keyfile.c_str(), rsa_key, &srcstat );
                } else if( VERBOSE(1) ) {
                    std::cerr<<opname<<": skipped unchanged file: "<<srcname<<std::endl;
                }
            } else {
                if( VERBOSE(1) )
                    std::cerr<<opname<<" directory: "<<srcname<<std::endl;

                real_dir_encrypt( src.c_str(), trim_offset, dst_dir, key_dir, rsa_key, op, opname, dstnameop,
                        keynameop );
            }
        } catch( const delayed_error & ) {
            error=true;
        } catch( const rscerror &err ) {
            std::cerr<<"Error in encryption of "<<srcname<<": "<<err.error()<<std::endl;
            error=true;
        }
    }

    if( error ) {
        throw delayed_error();
    }
}

static void recurse_dir_enc( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, int src_offset, bool op_handle_dir, const char *opname, namefunc dstname,
	namefunc keyname )
{
    autodir dir(src_dir);

    struct dirent *ent;
    bool error=false;

    while( (ent=dir.read())!=NULL ) {
        try {
            std::string src_filename(autofd::combine_paths(src_dir, ent->d_name));

            struct stat status, dststat;
            status=autofd::lstat( src_filename.c_str() );
            std::string dst_filename(dstname(dst_dir, src_filename.c_str()+src_offset, status.st_mode));
            std::string key_filename(keyname(key_dir, src_filename.c_str()+src_offset, status.st_mode));

            if( dst_filename.length()>0 ) {
                switch( status.st_mode & S_IFMT ) {
                case S_IFREG:
                    // Regular file
                    {
                        bool statsuccess=true;
                        try {
                            dststat=autofd::lstat( dst_filename.c_str() );
                        } catch( const rscerror & ) {
                            statsuccess=false;
                        }
                        if( !EXISTS(changed) || !statsuccess || abs(dststat.st_mtime-status.st_mtime)>VAL(mod_win) ) {
                            // Report to stderr (if applicable) the operation
                            if( VERBOSE(1) && opname!=NULL )
                                std::cerr<<opname<<" "<<src_filename<<std::endl;

                            try {
                                op( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(),
                                        rsa_key, &status );
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
                            recurse_dir_enc( src_filename.c_str(), dst_dir, key_dir, rsa_key, op,
                                    src_offset, op_handle_dir, opname, dstname, keyname );
                        } else {
                            recurse_dir_enc( src_filename.c_str(), dst_dir, key_dir, rsa_key, op,
                                    src_offset, op_handle_dir, opname, dstname, keyname );
                            op( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(),
                                    rsa_key, &status );
                        }
                    }
                    break;
                default:
                    // Unhandled type
                    std::cerr<<"Skipping unhandled file type: "<<src_filename<<std::endl;
                    break;
                }
            }
        } catch( const delayed_error & ) {
            error=true;
        } catch( const rscerror &err ) {
            std::cerr<<err.error()<<std::endl;
            error=true;
        }
    }

    if( error ) {
        throw delayed_error();
    }
}

static void file_delete( const char *source_file, const char *dst_file, const char *key_file,
        RSA *rsa_key, const struct stat *stat )
{
    struct stat status;

    try {
        status=autofd::lstat( dst_file );
    } catch( const rscerror &err ) {
        if( err.errornum()==ENOENT ) {
            // Need to erase file
            try {
                status=autofd::lstat( source_file );
            } catch( const rscerror &err ) {
                if( err.errornum()==ENOENT )
                    throw rscerror("Internal error", errno, source_file );
                else
                    throw rscerror("Can't stat file to delete", errno, source_file );
            }
            
            switch( status.st_mode & S_IFMT ) {
            case S_IFDIR:
                // Need to erase directory
                if( VERBOSE(1) ) {
                    if( EXISTS(delkey) )
                        std::cerr<<"Delete dirs "<<source_file<<", "<<key_file<<std::endl;
                    else
                        std::cerr<<"Delete dir "<<source_file<<std::endl;
                }

                // If an execution log is active - report the operation
                if( changes_log.get()!=NULL ) {
                    (*changes_log.get())<<source_file<<std::endl;
                }

                autofd::rmdir( source_file );
                if( EXISTS(delkey) )
                    autofd::rmdir( key_file );
                break;
            case S_IFREG:
#if defined S_IFLNK
            case S_IFLNK:
#endif
                if( VERBOSE(1) )
                    std::cerr<<"Delete "<<source_file<<std::endl;
                // If an execution log is active - report the operation
                if( changes_log.get()!=NULL )
                    (*changes_log.get())<<source_file<<std::endl;
                if( autofd::unlink( source_file )!=0 )
                    throw rscerror("Erasing file", errno, source_file );
                if( EXISTS(delkey) ) {
                    if( VERBOSE(1) )
                        std::cout<<"Delete "<<key_file<<std::endl;
                    autofd::unlink( key_file );
                }
                break;
            default:
                throw rscerror("Unhandled file type", 0, source_file );
            }
        } else
            throw ;
    }
}

static void real_dir_encrypt( const char *src_dir, size_t rel, const char *dst_dir, const char *key_dir, RSA *rsa_key,
                 encryptfunc op, const char *opname, namefunc dstname, namefunc keyname )
{
    // How many bytes of src_dir to skip when creating dirs under dst_dir
    int src_offset=calc_trim( src_dir+rel, VAL(trim) )+rel; 
    
    // Implement standard recursive descent on src_dir
    autofd::mkpath( dstname(dst_dir, src_dir+src_offset, S_IFDIR ).c_str(), 0777 );
    autofd::mkpath( keyname(key_dir, src_dir+src_offset, S_IFDIR ).c_str(), 0700 );
    
    recurse_dir_enc( src_dir, dst_dir, key_dir, rsa_key, op, src_offset, false, opname, dstname, keyname );
    
    if( EXISTS(del) ) {
        // "Scanning the encrypted files" takes whole different meaning based on whether we are
        // encrypting file names or not
        if( !EXISTS(nameenc) ) {
            std::string src_dst_name(src_dir, src_offset); // The name of the source string when used as dst
            std::string dst_src_name(dst_dir);
            int dst_src_offset=dst_src_name.length();
            dst_src_name=autofd::combine_paths(dst_src_name.c_str(), src_dir+src_offset);
            
            recurse_dir_enc( dst_src_name.c_str(), src_dst_name.c_str(), key_dir, rsa_key,
                file_delete, dst_src_offset, true, NULL, dstname, keyname );
        } else {
            // Scan the translation map rather than the encryption directory
            std::string src_dst_name(src_dir, src_offset); // The name of the source string when used as dst
            virt_recurse_dir_enc( dst_dir, src_dst_name.c_str(), key_dir, rsa_key,
                filemap::enc_file_delete, src_dir+src_offset );
        }
    }
}

// "Legacy" function - used when the double relatives of --filelist are not needed
void dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
                 encryptfunc op, const char *opname, namefunc dstname, namefunc keyname )
{
    real_dir_encrypt( src_dir, 0, dst_dir, key_dir, rsa_key, op, opname, dstname, keyname );
}

void file_encrypt( const char *source_file, const char *dst_file, const char *key_file,
        RSA *rsa_key, const struct stat *stat )
{
    // If an execution log is active - report the operation
    if( changes_log.get()!=NULL ) {
        (*changes_log.get())<<dst_file<<std::endl;
    }

    std::unique_ptr<key> head;
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
            head=std::unique_ptr<key>(key::read_key( static_cast<unsigned char *>(headmap.get()) ));

            if( ( EXISTS(fr) && ( head->get_sum_span()!=static_cast<uint32_t>(VAL(rollwin)) ||
                        head->get_sum_mod()!=static_cast<uint32_t>(VAL(rollsens)) ||
                        head->get_sum_min_dist()!=static_cast<uint32_t>(VAL(rollmin))) ) ||
                    ( EXISTS(fk) && head->get_key_size()!=static_cast<uint32_t>(VAL(keysize)) ) ) {
                headfd.clear();
            }
                
        }
        if( !headfd.valid() ) {
            head=key::new_key(key::CYPHER_AES, VAL(keysize), VAL(rollwin), VAL(rollsens), VAL(rollmin));
        }
    }

    int open_flags=O_RDONLY;
#ifdef HAVE_NOATIME
    switch( VAL(noatime) ) {
    case 1:
        if( stat!=NULL && stat->st_uid==geteuid() )
            open_flags|=O_NOATIME;
        break;
    case 2:
        open_flags|=O_NOATIME;
        break;
    }
#endif
    bool archive=!EXISTS(noarch);

    read_bufferfd infd;
    if( strcmp(source_file, "-")!=0 )
        infd=autofd(source_file, open_flags);
    else {
        infd=autofd::dup(STDIN_FILENO);
        // If source is stdin, there is nothing to archive
        archive=false;
    }

    autofd::mkpath( std::string(dst_file, autofd::dirpart(dst_file)).c_str(), 0777 );
    std::string tmpdst(dst_file);

    if( !EXISTS(risky_writes) ) {
        tmpdst+=CREATE_SUFFIX;
    }
    write_bufferfd outfd(autofd(tmpdst.c_str(), O_CREAT|O_TRUNC|O_RDWR, 0666));
    encrypt_file( head.get(), rsa_key, infd, outfd );
    if( !headfd.valid() ) {
        write_header( key_file, head.get() );
    }

    // Set the times on the encrypted file to match the plaintext file
    if( archive && stat!=NULL ) {
        copy_metadata( tmpdst.c_str(), stat );
    }

    if( !EXISTS(risky_writes) ) {
        autofd::mv( tmpdst.c_str(), dst_file );
    }
}

void file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key,
    const struct stat *stat )
{
    std::unique_ptr<key> head;
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
        head=read_header( headfd );
        headfd.clear();
    }

    read_bufferfd infd(autofd(src_file, O_RDONLY));
    status=infd.fstat();

    autofd::mkpath( std::string(dst_file, autofd::dirpart(dst_file)).c_str(), 0777);

    std::string tmpname(dst_file);

    if( !EXISTS(risky_writes) ) {
        tmpname+=CREATE_SUFFIX;
    }

    write_bufferfd outfd(autofd(tmpname.c_str(), O_CREAT|O_TRUNC|O_WRONLY, 0666));
    head=decrypt_file( std::move(head), rsa_key, infd, outfd );
    if( !headeread ) {
        write_header( key_file, head.get());
    }
    copy_metadata( tmpname.c_str(), &status );

    if( !EXISTS(risky_writes) ) {
        autofd::mv( tmpname.c_str(), dst_file );
    }
}

std::string name_concat( const char *left, const char *right, mode_t mode )
{
    if( right==NULL || *right=='\0' )
        return left;

    if( left==NULL || *left=='\0' )
        return right;

    return autofd::combine_paths( left, right );
}

std::string cond_name_concat( const char *left, const char *right, bool abs )
{
    if( abs )
        return right;

    return name_concat( left, right, 0 );
}

std::string left_only_concat( const char *left, const char *right, bool abs )
{
    return left;
}
