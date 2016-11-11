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
static void real_dir_encrypt( const TCHAR *src_dir, size_t rel, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
                 encryptfunc op, const TCHAR *opname, namefunc dstname, namefunc keyname );

static void copy_metadata( const TCHAR *destfilename, const struct stat *data )
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
	throw rscerror(_T("Setting time failed"), errno, destfilename );
}

int calc_trim( const TCHAR *path, int trim_count )
{
    int ret=0;

    if( path[0]==_T('\0') )
        throw rscerror(_T("Cannot trim empty path"));

    // Advance to the last character of the root element
    if( autofd::is_absolute( path ) ) {
        while( path[ret]!=_T(DIRSEP_C) )
            ret++;
    }

    if( trim_count==0 ) {
        // Even if the trim is 0, we still want to seperate out the leading slashes
        int i;
        for( i=0; path[i]==_T(DIRSEP_C); ++i )
            ;

        return i;
    }

    do {
        if( (path[ret]==_T(DIRSEP_C) || path[ret]==_T('\0')) && ret!=0 && path[ret-1]!=_T(DIRSEP_C) )
            trim_count--;
    } while( trim_count>0 && path[ret++]!=_T('\0') );

    if( trim_count>0 )
        throw rscerror(_T("Not enough directories to trim"));

    // Skip trailing slashes
    while( path[ret]==_T(DIRSEP_C) )
        ret++;

    return ret;
}

void filelist_encrypt( const TCHAR *src, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
        encryptfunc op, const TCHAR *opname, namefunc srcnameop, namefunc dstnameop, namefunc keynameop )
{
    autofd srcfd;

    if( _tcscmp(FILENAME(filelist), _T("-"))==0 ) {
        // Src is stdin
        srcfd=autofd::dup(STDIN_FILENO);
    } else {
        srcfd=autofd(FILENAME(filelist), O_RDONLY);
    }

    bool error=false;

    while( !srcfd.eof() ) {
        TSTRING srcname=srcfd.readline();

        if( srcname!=_T("") ) try {
            // Seperate the prefix from the actual name
            size_t src_offset=calc_trim( srcname.c_str(), VAL(trim) );
            TSTRING src_prefix(srcname.c_str(), src_offset);

            size_t trim_offset=0; // How much has been added to the source prefix

            // We may need to add to the prefix the relative directory
            if( !autofd::is_absolute( src_prefix.c_str() ) ) {
                trim_offset=src_prefix.length();
                src_prefix=autofd::combine_paths( FILENAME(src), src_prefix.c_str() );
                trim_offset=src_prefix.length()-trim_offset;
            }
            srcname=TSTRING(srcname.c_str()+src_offset);

            // Perform name (de)mangling
            TSTRING src=srcnameop( src_prefix.c_str(), srcname.c_str(), 0 );

            struct stat filestat=autofd::stat( src.c_str() );

            switch( filestat.st_mode&S_IFMT ) {
            case S_IFREG:
                {
                    TSTRING dstfile=dstnameop( dst_dir, srcname.c_str(), filestat.st_mode );
                    TSTRING keyfile=keynameop( key_dir, srcname.c_str(), filestat.st_mode );

                    struct stat dststat;
                    bool found=true;

                    try {
                        dststat=autofd::lstat( dstfile.c_str() );
                    } catch( const rscerror & ) {
                        found=false;
                    }
                    if( !EXISTS(changed) || !found ||
                            abs(dststat.st_mtime-filestat.st_mtime)>VAL(mod_win) )
                    {
                        // Report to stderr the operation
                        if( VERBOSE(1) )
                            std::cerr<<opname<<_T(" file: ")<<srcname<<std::endl;

                        op( src.c_str(), dstfile.c_str(), keyfile.c_str(), rsa_key, &filestat );
                    } else if( VERBOSE(1) ) {
                        std::cerr<<opname<<": skipped unchanged file: "<<srcname<<std::endl;
                    }
                }
                break;
            case S_IFDIR:
                {
                    if( VERBOSE(1) )
                        std::cerr<<opname<<" directory: "<<srcname<<std::endl;

                    // XXX What happens if there is an actual directory inside a meta-encrypted dir?
                    TSTRING dstfile=dstnameop( dst_dir, srcname.c_str(), filestat.st_mode );
                    TSTRING keyfile=keynameop( key_dir, srcname.c_str(), filestat.st_mode );

                    real_dir_encrypt( src.c_str(), trim_offset, dst_dir, key_dir, rsa_key, op, opname, dstnameop, keynameop );
                }
                break;
            default:
                std::cerr<<"Unsupported file type. Skipping "<<src<<std::endl;
                break;
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

static void recurse_dir_enc( const TCHAR *src_dir, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
        encryptfunc op, int src_offset, bool op_handle_dir, const TCHAR *opname, namefunc dstname,
	namefunc keyname )
{
    autodir dir(src_dir);

    struct dirent *ent;
    bool error=false;

    while( (ent=dir.read())!=NULL ) {
        try {
            TSTRING src_filename(autofd::combine_paths(src_dir, ent->d_name));

            struct stat status, dststat;
            status=autofd::lstat( src_filename.c_str() );
            TSTRING dst_filename(dstname(dst_dir, src_filename.c_str()+src_offset, status.st_mode));
            TSTRING key_filename(keyname(key_dir, src_filename.c_str()+src_offset, status.st_mode));

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
                    if( _tcscmp(ent->d_name,_T("."))!=0 && _tcscmp(ent->d_name,_T(".."))!=0 ) {
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

static void file_delete( const TCHAR *source_file, const TCHAR *dst_file, const TCHAR *key_file,
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
                    throw rscerror(_T("Internal error"), errno, source_file );
                else
                    throw rscerror(_T("Can't stat file to delete"), errno, source_file );
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
                autofd::unlink( source_file );
                if( EXISTS(delkey) ) {
                    if( VERBOSE(1) )
                        std::cout<<"Delete "<<key_file<<std::endl;

                    autofd::unlink( key_file );
                }
                break;
            default:
                throw rscerror(_T("Unhandled file type"), 0, source_file );
            }
        } else
            throw ;
    }
}

static void real_dir_encrypt( const TCHAR *src_dir, size_t rel, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
                 encryptfunc op, const TCHAR *opname, namefunc dstname, namefunc keyname )
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
            TSTRING src_dst_name(src_dir, src_offset); // The name of the source string when used as dst
            TSTRING dst_src_name(dst_dir);
            int dst_src_offset=dst_src_name.length();
            dst_src_name=autofd::combine_paths(dst_src_name.c_str(), src_dir+src_offset);
            
            recurse_dir_enc( dst_src_name.c_str(), src_dst_name.c_str(), key_dir, rsa_key,
                file_delete, dst_src_offset, true, NULL, dstname, keyname );
        } else {
            // Scan the translation map rather than the encryption directory
            TSTRING src_dst_name(src_dir, src_offset); // The name of the source string when used as dst
            virt_recurse_dir_enc( dst_dir, src_dst_name.c_str(), key_dir, rsa_key,
                filemap::enc_file_delete, src_dir+src_offset );
        }
    }
}

// "Legacy" function - used when the double relatives of --filelist are not needed
void dir_encrypt( const TCHAR *src_dir, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
                 encryptfunc op, const TCHAR *opname, namefunc dstname, namefunc keyname )
{
    real_dir_encrypt( src_dir, NULL, dst_dir, key_dir, rsa_key, op, opname, dstname, keyname );
}

void file_encrypt( const TCHAR *source_file, const TCHAR *dst_file, const TCHAR *key_file,
        RSA *rsa_key, const struct stat *stat )
{
    // If an execution log is active - report the operation
    if( changes_log.get()!=NULL ) {
        (*changes_log.get())<<dst_file<<std::endl;
    }

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

            if( ( EXISTS(fr) && ( head->get_sum_span()!=static_cast<uint32_t>(VAL(rollwin)) ||
                        head->get_sum_mod()!=static_cast<uint32_t>(VAL(rollsens)) ||
                        head->get_sum_min_dist()!=static_cast<uint32_t>(VAL(rollmin))) ) ||
                    ( EXISTS(fk) && head->get_key_size()!=static_cast<uint32_t>(VAL(keysize)) ) ) {
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
    if( _tcscmp(source_file, _T("-"))!=0 )
        infd=autofd(source_file, open_flags);
    else {
        infd=autofd::dup(STDIN_FILENO);
        // If source is stdin, there is nothing to archive
        archive=false;
    }

    autofd::mkpath( TSTRING(dst_file, autofd::dirpart(dst_file)).c_str(), 0777 );
    TSTRING tmpdst(dst_file);

    if( !EXISTS(risky_writes) ) {
        tmpdst+=_T(CREATE_SUFFIX);
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

void file_decrypt( const TCHAR *src_file, const TCHAR *dst_file, const TCHAR *key_file, RSA *rsa_key,
    const struct stat *stat )
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

    read_bufferfd infd(autofd(src_file, O_RDONLY));
    status=infd.fstat();

    autofd::mkpath( TSTRING(dst_file, autofd::dirpart(dst_file)).c_str(), 0777);

    TSTRING tmpname(dst_file);

    if( !EXISTS(risky_writes) ) {
        tmpname+=_T(CREATE_SUFFIX);
    }

    write_bufferfd outfd(autofd(tmpname.c_str(), O_CREAT|O_TRUNC|O_WRONLY, 0666));
    head=std::auto_ptr<key>(decrypt_file( head.get(), rsa_key, infd, outfd ));
    if( !headeread ) {
        write_header( key_file, head.get());
    }
    copy_metadata( tmpname.c_str(), &status );

    if( !EXISTS(risky_writes) ) {
        autofd::mv( tmpname.c_str(), dst_file );
    }
}

TSTRING name_concat( const TCHAR *left, const TCHAR *right, mode_t mode )
{
    if( right==NULL || *right==_T('\0') )
        return left;

    if( left==NULL || *left==_T('\0') )
        return right;

    return autofd::combine_paths( left, right );
}