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
#include "crypto.h"
#include "autodir.h"

void usage()
{
    fprintf(stderr, "Usage: " PACKAGE_NAME " <src> <dst> <keys> <publickey file>\n"
            "Options:\n"
            "-h                   Help - this page\n"
            "-d                   Decrypt.\n"
            "-r                   <plain> <cypher> and <keys> are all directory names. The encryption\n"
            "                     will apply to all files in them recursively\n"
            "-b keysize           Must be one of 128, 192 or 256 bits. Not valid for decryption.\n"
            "--roll-win           Rollover window size. Default is 8192 byte\n"
            "--roll-min           Minimal number of guaranteed non-rolled bytes. Default 8192.\n"
            "--roll-sensitivity   How sensitive are we to cutting a block. Default is \"roll-win\"\n"
            "--fr                 Force new rollover parameters, even if previous encryption used a\n"
            "                     different setting.\n"
            "--fk                 Force new key size, even if previous encryption used a different\n"
            "                     setting\n"
            "--no-archive-mode    Do not try to preserve timestamps, permissions etc.\n"
            "--gzip               path to gzip program to use\n\n"
            "Currently only AES encryption is supported\n");

    exit(0);
}

startup_options options;

int parse_cmdline( int argc, char *argv[] )
{
    enum option_type { ROLL_WIN=1, ROLL_MIN, ROLL_SENS, FR, FK, GZIP, NO_ARCHIVE };
    int c;
    const struct option long_options[]={
	{ "roll-win", 1, NULL, ROLL_WIN },
	{ "roll-min", 1, NULL, ROLL_MIN },
	{ "roll-sensitivity", 1, NULL, ROLL_SENS },
	{ "fr", 0, NULL, FR },
	{ "fk", 0, NULL, FK },
	{ "gzip", 1, NULL, GZIP },
        { "help", 0, NULL, 'h' },
        { "verbose", 0, NULL, 'v' },
        { "no-archive-mode", 0, NULL, NO_ARCHIVE },
	{ NULL, 0, NULL, 0 }};
    
    while( (c=getopt_long(argc, argv, "b:dhrv", long_options, NULL ))!=-1 )
    {
        switch(c) {
        case 'h':
            usage();
            break;
        case 'b':
            if( options.keysize!=0 ) {
                // Can't say "-b" twice
                throw rscerror("-b option specified twice");
            }
            options.keysize=atol(optarg);
            if( options.keysize==0 ) {
                // Invalid option
                throw rscerror("Invalid -b parameter given");
            }
            break;
        case 'd':
            if( options.decrypt ) {
                throw rscerror("-d option given twice");
            }
            options.decrypt=true;
            break;
        case 'v':
            options.verbosity++;
            break;
        case 'r':
            if( options.recurse ) {
                throw rscerror("-r option given twice");
            }
            options.recurse=true;
            break;
        case ROLL_WIN:
            if( options.rollwin!=0 )
                throw rscerror("--roll-win option given twice");
            options.rollwin=strtoul(optarg, NULL, 10);
            if( options.rollwin==0 )
                throw rscerror("Invalid --roll-win parameter given");
            break;
        case ROLL_MIN:
            if( options.rollmin!=0 )
                throw rscerror("--roll-min option given twice");
            options.rollmin=strtoul(optarg, NULL, 10);
            if( options.rollmin==0 )
                throw rscerror("Invalid --roll-min parameter given");
            break;
        case ROLL_SENS:
            if( options.rollsens!=0 )
                throw rscerror("--roll-sensitivity option given twice");
            options.rollsens=strtoul(optarg, NULL, 10);
            if( options.rollsens==0 )
                throw rscerror("Invalid --roll-sensitivity parameter given");
            break;
        case FR:
            if( options.fr )
                throw rscerror("--fr option given twice");
            options.fr=true;
            break;
        case FK:
            if( options.fk )
                throw rscerror("--fr option given twice");
            options.fr=true;
            break;
        case GZIP:
            if( options.gzip!=NULL )
                throw rscerror("--gzip option given twice");
            options.gzip=optarg;
            break;
        case NO_ARCHIVE:
            if( !options.archive )
                throw rscerror("--no-archive option given twice");
            options.archive=false;
            break;
        case '?':
            throw rscerror("Unrecognized option given");
            break;
        default:
            throw rscerror("Internal parameter processing error");
            break;
        }
    }

    // Some sanity check of the options
    if( options.decrypt ) {
        if( options.keysize!=0 )
            throw rscerror("Cannot specify key size for decryption");
        if( options.fr || options.fk )
            throw rscerror("\"force\" options incompatible with -d option");
        if( strcmp(argv[optind], "-")==0 ) {
            // Plaintext file is standard input/output
            if( options.archive ) {
                throw rscerror("Must use \"--no-archive-mode\" if plaintext file is stdin");
            }
        }
    }

    // Apply default values
    if( options.rollwin==0 )
        options.rollwin=8192;
    if( options.rollmin==0 )
        options.rollmin=8192;
    if( options.rollsens==0 )
        options.rollsens=options.rollmin;

    return optind;
}

void copy_metadata( const char *destfilename, const struct stat *data )
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
	throw rscerror(errno);
}

typedef int (* encryptfunc)(const char *source, const char *dest, const char *key, RSA *rsa);

static int recurse_dir_enc( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, int src_offset )
{
    int ret;

    autodir dir(opendir( src_dir ));

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
        stat( src_filename.c_str(), &status );
        switch( status.st_mode & S_IFMT ) {
        case S_IFREG:
            // Regular file
            
            break;
        case S_IFDIR:
            // Directory
            if( strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0 ) {
                if( mkdir( dst_filename.c_str(), status.st_mode )!=0 && errno!=EEXIST ||
                        mkdir( key_filename.c_str(), status.st_mode )!=0 && errno!=EEXIST )
                    throw rscerror(errno);

                recurse_dir_enc( src_filename.c_str(), dst_filename.c_str(), key_filename.c_str(), rsa_key, op,
                        src_offset );
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
int dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op )
{
    int ret=0;
    int src_offset=0; // How many bytes of src_dir to skip when creating dirs under dst_dir

    // Sanitize dst_dir
    do {
        switch( src_dir[src_offset] ) {
        case '/':
            src_offset++;
            break;
        case '.':
            {
                switch( src_dir[src_offset+1] ) {
                case '.':
                    if( src_dir[src_offset+2]=='/' || src_dir[src_offset+2]=='\0' ) {
                        src_offset+=3;
                    }
                    break;
                case '/':
                    src_offset+=2;
                    break;
                case '\0':
                    src_offset++;
                    break;
                }
            }
            break;
        }
    } while( src_dir[src_offset]=='/' );

    // Implement standard recursive descent on src_dir
    if( mkdir( (std::string(dst_dir)+"/"+(src_dir+src_offset)).c_str(), S_IRWXU|S_IRGRP|S_IXGRP )!=0 &&
            errno!=EEXIST ||
            mkdir( (std::string(key_dir)+"/"+(src_dir+src_offset)).c_str(), S_IRWXU|S_IRGRP|S_IXGRP )!=0 &&
            errno!=EEXIST )
        throw rscerror(errno);
    ret=recurse_dir_enc( src_dir, dst_dir, key_dir, rsa_key, op, src_offset );

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
        infd=autofd(open(source_file, open_flags));
    else
        infd=autofd(dup(STDIN_FILENO));
    autofd outfd(open(dst_file, O_CREAT|O_TRUNC|O_RDWR, status.st_mode));
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

int main( int argc, char *argv[] )
{
    int ret=0;
    
    ERR_load_crypto_strings();

    try {
        int argskip=parse_cmdline( argc, argv );
        argv+=argskip;
        argc-=argskip;

        if( argc!=4 )
            usage();

        RSA *rsa_key=extract_private_key(argv[3]);
        if( rsa_key==NULL ) {
            rsa_key=extract_public_key(argv[3]);
        }

        encryptfunc op;

        if( options.decrypt )
            op=file_decrypt;
        else
            op=file_encrypt;

        if( options.recurse ) {
            ret=dir_encrypt(argv[0], argv[1], argv[2], rsa_key, op);
        } else {
            ret=op(argv[0], argv[1], argv[2], rsa_key);
        }
    } catch( const rscerror &err ) {
        std::cerr<<err.error()<<std::endl;
        ret=1;
    }

    return ret;
}
