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
#include <stdint.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include "key_header.h"
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
    size_t key_size; /* key size IN BYTES! */
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

static size_t header_length( const struct key_header *header ) {
    return sizeof(struct key_header_aes)+header->key_size;
}

/* Generate a new AES file header. Make up an IV and key for the file */
struct key_header *gen_header(int key_length, enum CYPHER_TYPE cypher);
#if 0
{
    struct key_header_aes *header;
    int key_length_bytes=(key_length+7)/8;
    
    header=malloc(sizeof(struct key_header_aes)+key_length_bytes);
    if( header!=NULL ) {
        bzero(header, sizeof(header)+key_length_bytes);
        header->header.version=VERSION_MAGIC_1;
        header->header.cypher=cypher;
        header->header.key_size=key_length_bytes;
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
#endif

struct key_header *read_header( int headfd )
{
    const struct key_header *buffer;
    struct key_header *newheader;
    struct stat64 stat;
    
    if( fstat64(headfd, &stat)!=0 ) {
        perror("Couldn't stat encryption header file");
        return NULL;
    }

    if( stat.st_size<sizeof(*buffer) ) {
        fprintf(stderr, "encryption header file corrupt\n");
        return NULL;
    }
    
    buffer=mmap64(NULL, stat.st_size, PROT_READ, MAP_SHARED, headfd, 0 );
    if( buffer->version==VERSION_MAGIC_1 ) {
        newheader=malloc(stat.st_size);
        if( newheader!=NULL ) {
            memcpy( newheader, buffer, stat.st_size );
        }
        munmap((void *)buffer, stat.st_size);
    } else {
        fprintf(stderr, "Wrong magic in encryption header file\n");
        return NULL;
    }
    
    return newheader;
}

int write_header( int headfd, struct key_header *head )
{
    struct key_header_aes *aes_header=(void *)head;

    return write(headfd, aes_header, header_length(head))==header_length(head);
}

/* Encrypt the file's header */
int encrypt_header( const struct key_header *header, RSA *rsa, unsigned char *to )
{
    int keysize=RSA_size(rsa);
    unsigned char iv[AES_BLOCK_SIZE];
    AES_KEY aeskey;
    const struct key_header_aes *aes_header=(const void *)header;
    int i;
    
    assert(header_length(header)<=keysize);

    /* Create the padding data */
    /* Use the 1's complement of the file's IV for the padding data */
    for( i=0; i<AES_BLOCK_SIZE; ++i )
        iv[i]=~aes_header->iv[i];
    
    AES_set_encrypt_key(aes_header->key, header->key_size*8, &aeskey);
    bzero(to, keysize);
    AES_cbc_encrypt(to, to, keysize, &aeskey, iv, AES_ENCRYPT );

    /* Now place the header over it */
    memcpy(to, aes_header, header_length(header));

    /* Encrypt the whole thing in place */
    RSA_public_encrypt(keysize, to, to, rsa, RSA_NO_PADDING);

    return keysize;
}

/* Decrypt the file's header */
const struct key_header *decrypt_header( int fromfd, RSA *prv )
{
    size_t key_size=RSA_size(prv);
    struct key_header *decrypted_buff=malloc(key_size);
    unsigned char *verify_buff=NULL;
    unsigned char *filemap=mmap(NULL, key_size, PROT_READ, MAP_SHARED, fromfd, 0);
    int ok=0;

    RSA_private_decrypt(key_size, filemap, (unsigned char *)decrypted_buff, prv,
            RSA_NO_PADDING);

    if( decrypted_buff->version==VERSION_MAGIC_1 ) {
        verify_buff=malloc(key_size);
        encrypt_header( decrypted_buff, prv, verify_buff );

        int i;
        ok=1;
        for( i=0; ok && i<key_size; ++i )
            if( verify_buff[i]!=filemap[i] )
                ok=0;
    }
    
    munmap(filemap, key_size);
    free(verify_buff);
    if( !ok ) {
        free(decrypted_buff);
        decrypted_buff=NULL;
    }

    return decrypted_buff;
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
    }

    int numread;
    unsigned int start_position=0, end_position=0; /* Position along cyclic buffer */
    unsigned int numencrypted=0; /* Number of bytes encrypted without restarting from IV */
    unsigned long sum=0;
    int rollover=1; /* Whether we need to restart the encryption */
    size_t buffer_size=header->restart_buffer*2;
    /* make sure buffer_size is a multiple of BLOCK_SIZE */
    buffer_size+=(AES_BLOCK_SIZE-(buffer_size%AES_BLOCK_SIZE))%AES_BLOCK_SIZE;
    unsigned char *buffer=malloc(buffer_size);
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char encrypted[AES_BLOCK_SIZE];
    unsigned char numbytes=0;
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
        if( numencrypted>=header->restart_buffer )
            sum-=buffer[MOD_SUB(end_position,header->restart_buffer,buffer_size)];

        end_position=MOD_ADD(end_position,1,buffer_size);

        if( numencrypted>=header->min_norestart && sum%header->sum_mod==0 ) {
            /* The sum zeroed out - need to restart another block */
            rollover=1;
            numencrypted=0;
        }

        numbytes=MOD_SUB(end_position, start_position, buffer_size);
        if( numbytes>=AES_BLOCK_SIZE || rollover ) {
            /* Time to encrypt another block */
            AES_cbc_encrypt(buffer+start_position, encrypted, numbytes, &aeskey, iv, AES_ENCRYPT );
            write( tofd, encrypted, AES_BLOCK_SIZE );
            if( !rollover ) {
                start_position=MOD_ADD(start_position, AES_BLOCK_SIZE, buffer_size);
                numencrypted+=AES_BLOCK_SIZE;
            } else {
                numencrypted=0;
                start_position=0;
                end_position=0;
                sum=0;
            }
        }
    }

    if( numbytes!=0 ) {
        /* There are still leftover bytes to encrypt */
        AES_cbc_encrypt(buffer+start_position, buffer+start_position, numbytes, &aeskey,
                iv, AES_ENCRYPT );
        write( tofd, buffer+start_position, AES_BLOCK_SIZE );
    }

    /* Write out how many bytes of the last block were actual data */
    buffer[0]=numbytes%AES_BLOCK_SIZE;
    memcpy(iv, aes_header->iv, sizeof(iv) );
    AES_cbc_encrypt(buffer, buffer, 1, &aeskey, iv, AES_ENCRYPT );
    write( tofd, buffer, AES_BLOCK_SIZE );

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

struct key_header *decrypt_file( const struct key_header *header, RSA *prv, int fromfd,
        int tofd )
{
    if( header==NULL ) {
        /* Need to reconstruct the header from the encrypted file */
        header=decrypt_header( fromfd, prv );
    }

    /* If file does not contain a valid header - abort */
    if( header!=NULL ) {
        const struct key_header_aes *aes_header=(const void *)header;
        int child_pid;
        struct stat64 filestat;
        off64_t currpos;

        fstat64( fromfd, &filestat );

        /* Skip the header */
        currpos=lseek64(fromfd, RSA_size(prv), SEEK_SET);

        /* pipe, fork and run gzip */
        int iopipe[2];
        pipe(iopipe);
        switch(child_pid=fork())
        {
        case 0:
            /* child:
             * Redirect stdout to the file, and gunzip from the pipe */
            close(iopipe[1]);
            dup2(iopipe[0],0);
            close(iopipe[0]);
            dup2(tofd, 1);
            close(tofd);
            close(fromfd);
            execlp("gzip", "gzip", "-d", (char *)NULL);
            exit(1);
            break;
        case -1:
            /* Running gzip failed */
            free(header);
            return NULL;
        default:
            /* Parent */
            close(iopipe[0]);
        }

        int numread;
        unsigned int numdecrypted=0;
        unsigned int sum=0;
        unsigned int position=0;
        int error=0, rollover=1;
        size_t buffer_size=header->restart_buffer*2;
        buffer_size+=(AES_BLOCK_SIZE-(buffer_size%AES_BLOCK_SIZE))%AES_BLOCK_SIZE;
        unsigned char *buffer=malloc(buffer_size);
        unsigned char iv[AES_BLOCK_SIZE];
        AES_KEY aeskey;
        int done=0;
        
        AES_set_decrypt_key(aes_header->key, header->key_size*8, &aeskey);

        /* Read the file one AES_BLOCK_SIZE at a time, decrypt and write to the pipe */
        while((numread=read(fromfd, buffer+position, AES_BLOCK_SIZE))==AES_BLOCK_SIZE && !error && !done ) {
            currpos+=numread;
            if( rollover ) {
                memcpy( iv, aes_header->iv, sizeof(iv) );
                rollover=0;
            }
            
            AES_cbc_encrypt(buffer+position, buffer+position, AES_BLOCK_SIZE, &aeskey, iv, AES_DECRYPT );

            int i;
            for(i=0; i<AES_BLOCK_SIZE && !rollover; ++i) {
                sum+=buffer[position+i];
                if( numdecrypted>=header->restart_buffer ) {
                    sum-=buffer[MOD_SUB(position, header->restart_buffer, buffer_size)];
                }
                
                if( numdecrypted>=header->min_norestart && sum%header->sum_mod==0 ) {
                    rollover=1;
                }
            }

            if( currpos==filestat.st_size-AES_BLOCK_SIZE )
                done=1;
            else {
                /* Write the decrypted set to the pipe */
                write( iopipe[1], buffer+position, i );
                numdecrypted+=i;
            }

            if( !rollover || done ) {
                position=MOD_ADD(position,AES_BLOCK_SIZE,buffer_size);
            } else {
                while( i<AES_BLOCK_SIZE && !error ) {
                    /* If a block was interrupted, the remaining bytes should be zero */
                    error=(buffer[position+i]!=0);
                    ++i;
                }
                position=0;
                numdecrypted=0;
            }
        }

        if( !error && done ) {
            memcpy( iv, aes_header->iv, sizeof(iv) );
            AES_cbc_encrypt(buffer+position, buffer+position, 1, &aeskey, iv, AES_DECRYPT );
            if( buffer[position] )
                write( iopipe[1], buffer+MOD_SUB(position, AES_BLOCK_SIZE, buffer_size), buffer[position] );
        }
        
        close( iopipe[1] );

        int child_status;
        do {
            wait(&child_status);
        } while( !WIFEXITED(child_status) );
        
        if( !error && WEXITSTATUS(child_status)!=0 )
            error=1;

        if( error ) {
            /* Error in encrypted stream */
            free(header);
            header=NULL;
            fprintf(stderr, "Error in encrypted stream\n");
        }
    }

    return (struct key_header *)header;
}
