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
#include <sys/mman.h>
#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include "crypto.h"

/*
 * RAND_bytes should work right off the bat. If not, call
 * RAND_status first.
 */

#define VERSION_MAGIC_1 0xD657EA1Cul

struct key_header {
    unsigned long version;
    enum CYPHER_TYPE cypher;
    int key_size; /* key size IN BYTES! */
};


struct key_header_aes {
    struct key_header header;
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char key[0];
    /* char iv[AES_BLOCK_SIZE]; NOTE: This is the IV for the PADDING. It is not encrypted, but is stored in the plain file after
     * the key
     * XXX this is currently not used. Find out whether we want predictable padding */
};

/* Public/Private key handling */
RSA *extract_public_key( const char *pem_filename )
{
    BIO *in;
    X509 *x509;
    EVP_PKEY *pkey;
    RSA *rsa=NULL;
    
    /* We pull the public key out of the certificate. It's much like pulling teeth */
    /* First, get the certificate loaded into a stream */
    in=BIO_new(BIO_s_file()); /* XXX NULL is error */
    BIO_read_filename(in, pem_filename); /* XXX <=0 is error */
    /* Next, extract the X509 certificate from it */
    x509=PEM_read_bio_X509(in, NULL, NULL, NULL );
    /* And the public key in generic format */
    pkey=X509_get_pubkey(x509);
    /* And finally, we get the actual RSA key */
    rsa=EVP_PKEY_get1_RSA(pkey);
    /* Lastly, release all the resources we've allocated */
    X509_free(x509);
    EVP_PKEY_free(pkey);
    BIO_free_all(in);

    return rsa;
}

RSA *extract_private_key( const char *key_filename )
{
    BIO *in;
    RSA *rsa=NULL;
    
    /* We pull the public key out of the certificate. It's much like pulling teeth */
    /* First, get the certificate loaded into a stream */
    in=BIO_new(BIO_s_file()); /* XXX NULL is error */
    BIO_read_filename(in, key_filename); /* XXX <=0 is error */
    /* And finally, we get the actual RSA key */
    rsa=PEM_read_bio_RSAPrivateKey(in,NULL,NULL,NULL);
    /* Lastly, release all the resources we've allocated */
    BIO_free_all(in);

    return rsa;
}
/* Generate a new AES file header. Make up an IV and key for the file */
struct key_header *gen_header(int key_length, enum CYPHER_TYPE cypher)
{
    struct key_header_aes *header;
    
    header=malloc(sizeof(struct key_header_aes)+key_length+AES_BLOCK_SIZE);
    if( header!=NULL ) {
        header->header.version=VERSION_MAGIC_1;
        header->header.cypher=cypher;
        header->header.key_size=(key_length+7)/8;

        if( !RAND_bytes(header->key, header->header.key_size + AES_BLOCK_SIZE) )
        {
            free(header);
            header=NULL;
        }
    }

    return (struct key_header *)header;
}

/* Encrypt the file's header */
int encrypt_header( const struct key_header *header, RSA *rsa, unsigned char *to )
{
    int keysize=RSA_size(rsa);
    unsigned char iv[AES_BLOCK_SIZE];
    AES_KEY aeskey;
    const struct key_header_aes *aes_header=(const void *)header;
    
    assert((sizeof(struct key_header_aes)+header->key_size)<=keysize);

    /* Create the padding data */
    memcpy(iv, aes_header->key+header->key_size, sizeof(iv));
    AES_set_encrypt_key(aes_header->key, header->key_size*8, &aeskey);
    bzero(to, keysize);
    AES_cbc_encrypt(to, to, keysize, &aeskey, iv, AES_ENCRYPT );

    /* Now place the header over it */
    memcpy(to, aes_header, sizeof(*aes_header)+header->key_size);

    /* Encrypt the whole thing in place */
    RSA_public_encrypt(keysize, to, to, rsa, RSA_NO_PADDING);

    return keysize;
}

int encrypt_file( const struct key_header *header, RSA *rsa, int fromfd, int tofd )
{
    size_t PAGESIZE=sysconf(_SC_PAGESIZE);
    size_t key_size=RSA_size(rsa);
    size_t block_size;
    struct stat64 in_stat, out_stat;

    /* Create the file in it's minimal end required length */
    fstat64(fromfd, &in_stat);
    lseek64(tofd, in_stat.st_size+key_size-1, SEEK_SET);
    write(tofd, &in_stat, 1);

    /* Calculate the block size for our operations. It will be the maximum of PAGESIZE
     * and st_blksize */
    fstat64(tofd, &out_stat);
    block_size=out_stat.st_blksize>PAGESIZE ? out_stat.st_blksize : PAGESIZE;

    /* Write out the encrypted header */
    unsigned char *to=mmap64(NULL, block_size*2, PROT_READ|PROT_WRITE, MAP_SHARED, tofd,
            0 );
    unsigned char *from=mmap64(NULL, block_size*2, PROT_READ|PROT_WRITE, MAP_PRIVATE,
            fromfd, 0 );
    off64_t fromoff=0, tooff=0;

    tooff+=encrypt_header(header, rsa, to);

    while( fromoff<in_stat.st_size ) {
    }

    return 0;
}

#if 0
    {
        unsigned char secondbuff[2048];
        int i;
        
        RSA *decryptkey=extract_private_key("tests/test.key");
        
        /* Let's see if decrypting the encryption yields the same results */
        RSA_private_decrypt(keysize, to, secondbuff, decryptkey, RSA_NO_PADDING);

        for( i=0; i<(sizeof(*aes_header)+header->key_size) &&
                ((unsigned char *)aes_header)[i]==secondbuff[i]; ++i )
            ;
        if( i==sizeof(*aes_header)+header->key_size )
            printf("success\n");
        else
            printf("Error\n");
    }
}
#endif
