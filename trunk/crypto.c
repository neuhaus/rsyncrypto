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
#include <sys/wait.h>
#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include "crypto.h"

#define CRYPT_RESTART_BUFFER 8192
#define CRYPT_MIN_NORESTART 8192
#define CRYPT_SUM_MOD 8192

/* Cyclic add and subtract */
#define MOD_ADD(a,b,mod) (((a)+(b))%(mod))
#define MOD_SUB(a,b,mod) MOD_ADD((a), (mod)-(b), (mod))

#define VERSION_MAGIC_1 0xD657EA1Cul

struct key_header {
    unsigned long version;
    enum CYPHER_TYPE cypher;
    int key_size; /* key size IN BYTES! */
    unsigned int restart_buffer, min_norestart, sum_mod; /* checksum recycle policy parameters */
};


struct key_header_aes {
    struct key_header header;
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char key[0];
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
    
    header=malloc(sizeof(struct key_header_aes)+key_length);
    if( header!=NULL ) {
        bzero(header, sizeof(header));
        header->header.version=VERSION_MAGIC_1;
        header->header.cypher=cypher;
        header->header.key_size=(key_length+7)/8;
        header->header.restart_buffer=CRYPT_RESTART_BUFFER;
        header->header.min_norestart=CRYPT_MIN_NORESTART;
        header->header.sum_mod=CRYPT_SUM_MOD;

        if( !RAND_bytes(header->key, header->header.key_size) ||
                !RAND_bytes(header->iv, AES_BLOCK_SIZE) )
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
    int i;
    
    assert((sizeof(struct key_header_aes)+header->key_size)<=keysize);

    /* Create the padding data */
    /* Use the 1's complement of the file's IV for the padding data */
    for( i=0; i<AES_BLOCK_SIZE; ++i )
        iv[i]=~aes_header->iv[i];
    
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
    size_t key_size=RSA_size(rsa);
    const struct key_header_aes *aes_header=(const void *)header;
    int child_pid;

    /* Skip the header. We'll only write it out once the file itself is written */
    lseek64(tofd, key_size, SEEK_SET);

    /* pipe, fork and run gzip */
    int iopipe[2];
    pipe(iopipe);
    switch(child_pid=fork())
    {
    case 0:
	/* child */
	/* Redirect stdout to the pipe, and gzip the fromfd */
	close(iopipe[0]);
	dup2(iopipe[1],1);
	close(iopipe[1]);
	dup2(fromfd, 0);
	close(fromfd);
	close(tofd);
	execlp("gzip", "gzip", "--rsyncable", (char *)NULL);
	exit(1);
	break;
    case -1:
        /* Running gzip failed */
        return -1;
    default:
        /* Parent */
        close(iopipe[1]);
        close(fromfd);
        fromfd=-1;
    }

    int numread;
    unsigned int start_position=0, end_position=0; /* Position along cyclic buffer */
    unsigned int numencrypted=0; /* Number of bytes encrypted without restarting from IV */
    unsigned long sum=0;
    int rollover=1; /* Whether we need to restart the encryption */
    size_t buffer_size=CRYPT_RESTART_BUFFER*2;
    /* make sure buffer_size is a multiple of BLOCK_SIZE */
    buffer_size+=(AES_BLOCK_SIZE-(buffer_size%AES_BLOCK_SIZE))%AES_BLOCK_SIZE;
    unsigned char *buffer=malloc(buffer_size);
    unsigned char iv[AES_BLOCK_SIZE];
    AES_KEY aeskey;
    AES_set_encrypt_key(aes_header->key, header->key_size*8, &aeskey);

    /* Read the pipe one byte at a time, block encrypt and write to the file */
    while((numread=read(iopipe[0], buffer+end_position, 1 ))>0) {
        if( rollover ) {
            /* Need to restart the encryption */
            memcpy( iv, aes_header->iv, AES_BLOCK_SIZE);
            rollover=0;
        }

        /* Update the rolling sum */
        sum=sum+buffer[end_position];
        if( numencrypted>=CRYPT_MIN_NORESTART )
            sum-=buffer[MOD_SUB(end_position,CRYPT_RESTART_BUFFER,buffer_size)];

        end_position=MOD_ADD(end_position,1,buffer_size);

        if( numencrypted>=CRYPT_MIN_NORESTART && sum%CRYPT_SUM_MOD==0 ) {
            /* The sum zeroed out - need to restart another block */
            rollover=1;
            numencrypted=0;
        }

        int numbytes=MOD_SUB(end_position, start_position, buffer_size);
        if( numbytes>=AES_BLOCK_SIZE || rollover ) {
            /* Time to encrypt another block */
            AES_cbc_encrypt(buffer+start_position, buffer+start_position, numbytes,
                    &aeskey, iv, AES_ENCRYPT );
            write( tofd, buffer+start_position, AES_BLOCK_SIZE );
            if( !rollover ) {
                start_position=MOD_ADD(start_position, AES_BLOCK_SIZE, buffer_size);
                numencrypted+=AES_BLOCK_SIZE;
            } else {
                numencrypted=0;
                start_position=0;
                end_position=0;
            }
        }
    }

    close(iopipe[0]);
    free(buffer);

    int childstatus;
    do {
        wait(&childstatus);
    } while( !WIFEXITED(childstatus) );

    if( WEXITSTATUS(childstatus)==0 ) {
        /* gzip was successful - write out the header, encrypted */
        buffer=mmap( NULL, key_size, PROT_READ|PROT_WRITE, MAP_SHARED, tofd, 0 );
        if( buffer!=NULL ) {
            encrypt_header( header, rsa, buffer );
            munmap( buffer, key_size );
        }
    } else {
        return WEXITSTATUS(childstatus);
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
