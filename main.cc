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
            "-b keysize           Must be one of 128, 256 or 512 bits.\n"
            "--roll-win           Rollover window size. Default is 256 byte\n"
            "--roll-min           Minimal number of guarenteed non-rolled bytes. Default 8192.\n"
            "--roll-sensitivity   How sensitive are we to cutting a block. Default is \"roll-win\"\n"
            "--fr                 Force new rollover parameters, even if previous encryption used a\n"
            "                     different setting.\n"
            "--fk                 Force new key size, even if previous encryption used a different\n"
            "                     setting\n"
            "--gzip               path to gzip program to use\n\n"
            "Currently only AES encryption is supported\n");

    exit(1);
}

struct options {
    const char *srcdir;
    const char *dstdir;
    const char *keysdis;
    const char *key;
};

int parse_cmdline( int argc, char *argv[] )
{
    enum options { ROLL_WIN=1, ROLL_MIN, ROLL_SENS, FR, FK, GZIP };
    int c;
    const struct option long_options[]={
	{ "roll-win", 1, NULL, ROLL_WIN },
	{ "roll-min", 1, NULL, ROLL_MIN },
	{ "roll-sensitivity", 1, NULL, ROLL_SENS },
	{ "fr", 0, NULL, FR },
	{ "fk", 0, NULL, FK },
	{ "gzip", 1, NULL, GZIP },
	{ NULL, 0, NULL, 0 }};
    
    while( (c=getopt_long(argc, argv, "b:", long_options, NULL ))!=-1 )
    {
        switch(c) {
        case 'b':
            break;
        }
    }

    return 0;
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

int main_enc( int argc, char * argv[] )
{
    std::auto_ptr<key> head;
    autofd headfd;
    struct stat status;

    // Read in the header, or generate a new one if can't
    {
        headfd=autofd(open( argv[3], O_RDONLY ));
        if( headfd!=-1 ) {
            autommap headmap( headfd, PROT_READ );
            head=std::auto_ptr<key>(key::read_key( static_cast<unsigned char *>(headmap.get()) ));
        } else {
            head=std::auto_ptr<key>(key::new_key());
        }
    }

    RSA *rsa=extract_public_key(argv[4]);
    autofd infd(open(argv[1], O_LARGEFILE|O_RDONLY
#ifdef HAVE_NOATIME
                |O_NOATIME
#endif
                ));
    fstat(infd, &status);
    autofd outfd(open(argv[2], O_LARGEFILE|O_CREAT|O_TRUNC|O_RDWR, status.st_mode));
    encrypt_file( head.get(), rsa, infd, outfd );
    if( headfd==-1 ) {
        write_header( argv[3], head.get() );
    }

    // Set the times on the encrypted file to match the plaintext file
    infd.release();
    outfd.release();
    copy_metadata( argv[2], &status );
    RSA_free(rsa);

    return 0;
}

int main_dec( int argc, char * argv[] )
{
    std::auto_ptr<key> head;
    // int infd, outfd, headfd;
    struct stat status;

    /* Decryption */
    autofd headfd(open( argv[3], O_RDONLY ));
    if( headfd!=-1 ) {
        head=std::auto_ptr<key>(read_header( headfd ));
        close(headfd);
    }
    /* headfd indicates whether we need to write a new header to disk. -1 means yes. */

    RSA *rsa=extract_private_key(argv[4]);
    if( rsa==NULL ) /* No private key - get public key instead */
    {
        rsa=extract_public_key(argv[4]);
    }
    autofd infd(open(argv[2], O_LARGEFILE|O_RDONLY), true);
    fstat(infd, &status);
    autofd outfd(open(argv[1], O_LARGEFILE|O_CREAT|O_TRUNC|O_WRONLY, status.st_mode), true);
    head=std::auto_ptr<key>(decrypt_file( head.get(), rsa, infd, outfd ));
    if( headfd==-1 ) {
        write_header( argv[3], head.get());
    }
    infd.release();
    outfd.release();
    copy_metadata( argv[1], &status );
    RSA_free(rsa);

    return 0;
}

int main( int argc, char *argv[] )
{
    int ret=0;
    
    ERR_load_crypto_strings();

    try {
        switch( argv[1][0] )
        {
            case 'e':
                return main_enc(argc-1, argv+1);
            case 'd':
                return main_dec(argc-1, argv+1);
            default:
                fprintf(stderr, "Prefix either \"d\" or \"e\" to the arguments to decrypt/encrypt\n");
        }
    } catch( const rscerror &err ) {
        std::cerr<<err.error()<<std::endl;
        ret=1;
    }

    return ret;
}
