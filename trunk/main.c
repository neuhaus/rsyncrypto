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

#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "crypto.h"

void usage()
{
    fprintf(stderr, "Usage: " PACKAGE_NAME " <sourcedir> <destdir> <keysdir> <publickey file>\n"
            "Options:\n"
            "-b keysize           Must be one of 128, 256 or 512 bits.\n"
            "--roll-win           Rollover window size. Default is 256 byte\n"
            "--roll-min           Minimal number of guarenteed non-rolled bytes. Default 8192.\n"
            "--roll-sensitivity   How sensitive are we to cutting a block. Default is \"roll-win\"\n"
            "-f                   Force new rollover parameters, even if previous encryption used a\n"
            "                     different setting.\n\n"
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
    int c;
    
    while( (c=getopt(argc, argv, "b:"))!=-1 )
    {
        switch(c) {
        case 'b':
            break;
        }
    }

    return 0;
}

int main( int argc, char * argv[] )
{
    struct key_header *head;
    int in, out;
    struct stat64 status;
    
    head=gen_header(128, CYPHER_AES);
    RSA *rsa=extract_public_key(argv[4]);
    /* encrypt_header(head, rsa, buffer ); */
    in=open(argv[1], O_LARGEFILE|O_RDONLY); /* Add O_NOATIME after proper configure tests */
    fstat64(in, &status);
    out=open(argv[2], O_LARGEFILE|O_CREAT|O_TRUNC|O_RDWR, status.st_mode);
    encrypt_file( head, rsa, in, out );
    free(head);

    return 0;
}
