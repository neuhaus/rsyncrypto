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
            "\t-b keysize\tMust be one of 128, 256 or 512 bits.\n\n"
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

    return 0;
}
