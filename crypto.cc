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
#include <fcntl.h>
#include <stdint.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include <iostream>

#include "rsyncrypto.h"

#include "crypto.h"

#define CRYPT_RESTART_BUFFER 8192
#define CRYPT_MIN_NORESTART 8192
#define CRYPT_SUM_MOD 8192

/* Cyclic add and subtract */
#define MOD_ADD(a,b,mod) (((a)+(b))%(mod))
#define MOD_SUB(a,b,mod) MOD_ADD((a), (mod)-(b), (mod))

#define VERSION_MAGIC_1 0xD657EA1Cul

#if 0
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
#endif

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

key *read_header( int headfd )
{
    autommap headmap( headfd, PROT_READ );
    return key::read_key( headmap.get_uc() );
}

void write_header( const char *filename, const key *head )
{
    autofd newhead(open(filename, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR), true);
    off64_t headsize=head->exported_length();
    if( lseek( newhead, headsize-1, SEEK_SET )!=headsize-1 ||
            write( newhead, &newhead, 1 )!=1 )
        throw rscerror(errno);

    autommap headfilemap( NULL, headsize, PROT_WRITE, MAP_SHARED, newhead, 0 );
    head->export_key( headfilemap.get() );
}

/* Encrypt the file's header */
int encrypt_header( const key *header, RSA *rsa, unsigned char *to )
{
    const size_t keysize=RSA_size(rsa);
    //unsigned char iv[AES_BLOCK_SIZE];
    //AES_KEY aeskey;
    
    assert(header->exported_length()<=keysize);

    header->pad_area( to, keysize );

    /* Now place the header over it */
    header->export_key( to );

    /* Encrypt the whole thing in place */
    RSA_public_encrypt(keysize, to, to, rsa, RSA_NO_PADDING);

    return keysize;
}

/* Decrypt the file's header */
key *decrypt_header( int fromfd, RSA *prv )
{
    const size_t key_size=RSA_size(prv);
    //auto_array<unsigned char> decrypted_buff(new unsigned char [key_size]);
    // unsigned char *verify_buff=NULL;
    autommap filemap(NULL, key_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fromfd, 0);
    //int ok=0;

    RSA_private_decrypt(key_size, filemap.get_uc(), filemap.get_uc(), prv, RSA_NO_PADDING);

    std::auto_ptr<key> ret(key::read_key( filemap.get_uc() ));

    // Let's verify that we have read the correct data from the file, by reencoding the key we got and comparing
    // the cyphertexts.
    // On second thought - let's not. You never know if we don't change encoding at some future point.

    return ret.release();
}

void encrypt_file( key *header, RSA *rsa, int fromfd, int tofd )
{
    const size_t key_size=RSA_size(rsa);
    int child_pid;

    /* Skip the header. We'll only write it out once the file itself is written */
    lseek64(tofd, key_size, SEEK_SET);

    /* pipe, fork and run gzip */
    autofd ipipe;
    {
        int iopipe[2];
        if( pipe(iopipe)!=0 )
            throw rscerror(errno);

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
            throw rscerror(errno);
            break;
        default:
            /* Parent */
            close(iopipe[1]);
            ipipe=autofd(iopipe[0]);
            break;
        }
    }

    // Run through gzip's output, and encrypt it
    const size_t block_size=header->block_size(); // Let's cache the block size
    auto_array<unsigned char> buffer(new unsigned char [block_size]);
    unsigned int i=0;
    int numread=1;
    bool new_block=true;

    while( (numread=ipipe.read(buffer.get()+i, 1))!=0 ) {
        if( new_block ) {
            header->init_encrypt();
            new_block=false;
        }

        new_block=header->calc_boundry( buffer[i] );
        i+=numread;
        if( i>=block_size || new_block ) {
            header->encrypt_block( buffer.get(), i );
            autofd::write( tofd, buffer.get(), block_size );

            i=0;
        }
    }
    
    if( i>0 ) {
        // Still some leftover bytes to encrypt
        header->encrypt_block( buffer.get(), i );
        autofd::write( tofd, buffer.get(), block_size );
    }

    // Report how many bytes of last block are relevant.
    bzero( buffer.get(), block_size );
    buffer[0]=i;
    header->init_encrypt();
    header->encrypt_block( buffer.get(), 1 );
    autofd::write( tofd, buffer.get(), block_size );

    // Wait for gzip to return, and check whether it succeeded
    int childstatus;
    do {
        waitpid(child_pid, &childstatus, 0);
    } while( !WIFEXITED(childstatus) );

    if( WEXITSTATUS(childstatus)==0 ) {
        /* gzip was successful - write out the header, encrypted */
        autommap buffer( NULL, key_size, PROT_READ|PROT_WRITE, MAP_SHARED, tofd, 0 );
        encrypt_header( header, rsa, buffer.get_uc() );
    } else {
        throw rscerror("Error in running gzip");
    }
}

key *decrypt_file( key *header, RSA *prv, int fromfd, int tofd )
{
    std::auto_ptr<key> new_header;
    if( header==NULL ) {
        /* Need to reconstruct the header from the encrypted file */
        new_header=std::auto_ptr<key>(decrypt_header( fromfd, prv ));
        header=new_header.get();
    }

    /* If file does not contain a valid header - abort */
    if( header==NULL )
        throw rscerror("Couldn't extract encryption header");

    int child_pid;
    struct stat64 filestat;
    off64_t currpos;

    fstat64( fromfd, &filestat );

    /* Skip the header */
    currpos=lseek64(fromfd, RSA_size(prv), SEEK_SET);

    autofd opipe;

    {
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
            throw rscerror("Couldn't create gzip process");
            break;
        default:
            /* Parent */
            close(iopipe[0]);
            opipe=autofd(iopipe[1]);
        }
    }

    size_t numread;
    const size_t block_size=header->block_size();
    auto_array<unsigned char> buffer(new unsigned char [block_size]);
    bool done=false;
    bool new_block=true;

    /* Read the file one AES_BLOCK_SIZE at a time, decrypt and write to the pipe */
    while( !done && (numread=autofd::read(fromfd, buffer.get(), block_size))!=0 ) {
        currpos+=numread;
        if( numread>0 && numread<block_size )
            throw rscerror("Unexpected file end");

        if( new_block ) {
            header->init_encrypt();
            new_block=false;
        }

        header->decrypt_block( buffer.get(), block_size );

        unsigned int i;
        for( i=0; i<block_size && !new_block; ++i ) {
            new_block=header->calc_boundry(buffer[i]);
        }

        if( currpos>=filestat.st_size-block_size ) {
            done=true;

            // Oops - file is not a whole multiple of block size
            if( currpos>filestat.st_size-block_size )
                throw rscerror("Uneven file end");
        } else {
            opipe.write( buffer.get(), i );
        }

        // If this was not a full block, the remaining bytes should be zero
        for( ; i<block_size; ++i )
            if( buffer[i]!=0 )
                throw rscerror("Error in encrypted stream");
    }
    
    // The next block will tell us how many bytes of the last block should be written.
    auto_array<unsigned char> buffer2(new unsigned char [block_size]);
    if( autofd::read(fromfd, buffer2.get(), block_size)!=static_cast<ssize_t>(block_size) )
        throw rscerror("Unexcpeted end of file past sanity checks");

    header->init_encrypt();
    header->decrypt_block( buffer2.get(), block_size );
    for( unsigned int i=1; i<block_size; ++i )
        if( buffer2[i]!=0 )
            throw rscerror("Error in encrypted stream (trailer)");
    if( buffer2[0]>block_size )
        throw rscerror("Error in encrypted stream (trailer 2)");

    opipe.write( buffer.get(), buffer2[0] );
    opipe.clear();

    int child_status;
    do {
        waitpid( child_pid, &child_status, 0 );
    } while( !WIFEXITED(child_status) );

    if( WEXITSTATUS(child_status)!=0 )
        throw rscerror("gunzip failed to run");

    new_header.release();
    
    return header;
}
