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

void usage()
{
    fprintf(stderr, "Usage: " PACKAGE_NAME " <sourcedir> <destdir> <keysdir> <publickey file>\n"
            "Options:\n"
            "-h                   Help - this page\n"
            "-d                   Decrypt.\n"
            "-b keysize           Must be one of 128, 256 or 512 bits. Not valid for decryption.\n"
            "--roll-win           Rollover window size. Default is 256 byte\n"
            "--roll-min           Minimal number of guarenteed non-rolled bytes. Default 8192.\n"
            "--roll-sensitivity   How sensitive are we to cutting a block. Default is \"roll-win\"\n"
            "--fr                 Force new rollover parameters, even if previous encryption used a\n"
            "                     different setting.\n"
            "--fk                 Force new key size, even if previous encryption used a different\n"
            "                     setting\n"
            "--gzip               path to gzip program to use\n\n"
            "Currently only AES encryption is supported\n");

    exit(0);
}

startup_options options;

int parse_cmdline( int argc, char *argv[] )
{
    enum option_type { ROLL_WIN=1, ROLL_MIN, ROLL_SENS, FR, FK, GZIP };
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
	{ NULL, 0, NULL, 0 }};
    
    while( (c=getopt_long(argc, argv, "b:dhv", long_options, NULL ))!=-1 )
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
    }

    // Apply default values
    if( options.rollwin==0 )
        options.rollwin=256;
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

int main_enc( int argc, char * args[] )
{
    std::auto_ptr<key> head;
    autofd headfd;
    struct stat status;

    if( argc!=4 ) {
        usage();
    }

    // Read in the header, or generate a new one if can't
    {
        headfd=autofd(open( args[2], O_RDONLY ));
        if( headfd!=-1 ) {
            autommap headmap( headfd, PROT_READ );
            head=std::auto_ptr<key>(key::read_key( static_cast<unsigned char *>(headmap.get()) ));

            if( options.fr && ( head->get_sum_span()!=options.rollwin ||
                        head->get_sum_mod()!=options.rollsens ||
                        head->get_sum_min_dist()!=options.rollmin) ) {
                headfd.clear();
            }
        }
        if( headfd==-1 ) {
            head=std::auto_ptr<key>(key::new_key());
        }
    }

    RSA *rsa=extract_public_key(args[3]);
    autofd infd(open(args[0], O_LARGEFILE|O_RDONLY
#ifdef HAVE_NOATIME
                |O_NOATIME
#endif
                ));
    fstat(infd, &status);
    autofd outfd(open(args[1], O_LARGEFILE|O_CREAT|O_TRUNC|O_RDWR, status.st_mode));
    encrypt_file( head.get(), rsa, infd, outfd );
    if( headfd==-1 ) {
        write_header( args[2], head.get() );
    }

    // Set the times on the encrypted file to match the plaintext file
    infd.release();
    outfd.release();
    copy_metadata( args[1], &status );
    RSA_free(rsa);

    return 0;
}

int main_dec( int argc, char * args[] )
{
    std::auto_ptr<key> head;
    // int infd, outfd, headfd;
    struct stat status;

    /* Decryption */
    autofd headfd(open( args[2], O_RDONLY ));
    if( headfd!=-1 ) {
        head=std::auto_ptr<key>(read_header( headfd ));
        close(headfd);
    }
    /* headfd indicates whether we need to write a new header to disk. -1 means yes. */

    RSA *rsa=extract_private_key(args[3]);
    if( rsa==NULL ) /* No private key - get public key instead */
    {
        rsa=extract_public_key(args[3]);
    }
    autofd infd(open(args[1], O_LARGEFILE|O_RDONLY), true);
    fstat(infd, &status);
    autofd outfd(open(args[0], O_LARGEFILE|O_CREAT|O_TRUNC|O_WRONLY, status.st_mode), true);
    head=std::auto_ptr<key>(decrypt_file( head.get(), rsa, infd, outfd ));
    if( headfd==-1 ) {
        write_header( args[2], head.get());
    }
    infd.release();
    outfd.release();
    copy_metadata( args[0], &status );
    RSA_free(rsa);

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
        if( !options.decrypt )
        {
            ret=main_enc(argc, argv);
        } else {
            ret=main_dec(argc, argv);
        }
    } catch( const rscerror &err ) {
        std::cerr<<err.error()<<std::endl;
        ret=1;
    }

    return ret;
}
