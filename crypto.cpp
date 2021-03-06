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
#include "crypto.h"
#include "process.h"
#include "autopipe.h"
#include "redir.h"

/* Cyclic add and subtract */
#define MOD_ADD(a,b,mod) (((a)+(b))%(mod))
#define MOD_SUB(a,b,mod) MOD_ADD((a), (mod)-(b), (mod))

#define VERSION_MAGIC_1 0xD657EA1Cul

/* Public/Private key handling */
RSA *extract_public_key( const char *pem_filename )
{
    BIO *in;
    X509 *x509;
    EVP_PKEY *pkey;
    RSA *rsa=NULL;
    
    /* We pull the public key out of the certificate. It's much like pulling teeth */
    /* First, get the certificate loaded into a stream */
    in=BIO_new(BIO_s_file()); /* NULL is error */
    if( in==NULL )
        throw rscerror( "Error allocating public key", ENOMEM );

    if( BIO_read_filename(in, pem_filename)<=0 ) /* <=0 is error */
        throw rscerror( "Error reading public key file", errno, pem_filename );

    /* Next, extract the X509 certificate from it */
    x509=PEM_read_bio_X509(in, NULL, NULL, NULL );
    if( x509==NULL )
        throw rscerror( "Error parsing certificate" );

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
    in=BIO_new(BIO_s_file()); /* NULL is error */
    if( in==NULL )
        throw rscerror( "Error allocating private key" );
    if( BIO_read_filename(in, key_filename)<=0 ) /* <=0 is error */
        throw rscerror( "Error reading private key file", errno, key_filename );
    
    /* And finally, we get the actual RSA key */
    rsa=PEM_read_bio_RSAPrivateKey(in,NULL,NULL,NULL);
    /* Lastly, release all the resources we've allocated */
    BIO_free_all(in);

    return rsa;
}

std::unique_ptr<key> read_header( const autofd &headfd )
{
    autommap headmap( headfd, PROT_READ );
    return key::read_key( headmap.get_uc() );
}

void write_header( const char *filename, const key *head )
{
    autofd::mkpath( std::string(filename, autofd::dirpart(filename)).c_str(), 0700 );
    autofd newhead(filename, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    off_t headsize=head->exported_length();

    if( newhead.lseek( headsize-1, SEEK_SET )!=headsize-1 ||
            newhead.write( filename, 1 )!=1 )
        throw rscerror("write failed", errno, filename );

    autommap headfilemap( NULL, static_cast<size_t>(headsize), PROT_WRITE|PROT_READ, MAP_SHARED,
        newhead, 0 );
    head->export_key( headfilemap.get() );
}

const uint32_t HEADER_ENCRYPTION_VERSION=0;

size_t header_size( const RSA *rsa )
{
    return RSA_size(rsa)+sizeof(HEADER_ENCRYPTION_VERSION);
}

/* Encrypt the file's header */
void encrypt_header( const key *header, RSA *rsa, unsigned char *to )
{
    size_t export_size=header->exported_length();

    *reinterpret_cast<uint32_t *>(to)=htonl(HEADER_ENCRYPTION_VERSION);

    to+=sizeof(HEADER_ENCRYPTION_VERSION);

    header->export_key( to );

    /* Encrypt the whole thing in place */
    if( RSA_public_encrypt(export_size, to, to, rsa, RSA_PKCS1_OAEP_PADDING)==-1 ) {
        unsigned long rsaerr=ERR_get_error();
        throw rscerror(ERR_error_string(rsaerr, NULL));
    }
}

/* Decrypt the file's header */
std::unique_ptr<key> decrypt_header( file_t fromfd, RSA *prv )
{
    const size_t key_size=RSA_size(prv);
    size_t headsize=header_size(prv);
    autommap filemap(NULL, headsize, PROT_READ, MAP_PRIVATE, fromfd, 0);

    if( *static_cast<uint32_t *>(filemap.get())!=htonl(HEADER_ENCRYPTION_VERSION) )
        throw rscerror("Wrong file or header encrypted with wrong encryption");

    unsigned char *buff=filemap.get_uc()+sizeof(HEADER_ENCRYPTION_VERSION);
    auto_array<unsigned char> decrypted(new unsigned char[headsize]);

    const BIGNUM *p, *q;
    RSA_get0_factors(prv, &p, &q);
    if( (p==nullptr || q==nullptr) ) {
        // This is not a private key!
        throw rscerror("Neither AES session key nor RSA private key present - cannot decrypt using only public key");
    }

    if( RSA_private_decrypt(key_size, buff, decrypted.get(), prv, RSA_PKCS1_OAEP_PADDING)==-1 ) {
        unsigned long rsaerr=ERR_get_error();
        throw rscerror(ERR_error_string(rsaerr, NULL));
    }

    std::unique_ptr<key> ret(key::read_key( decrypted.get() ));

    // Let's verify that we have read the correct data from the file, by reencoding the key we got and comparing
    // the cyphertexts.
    // On second thought - let's not. You never know if we don't change encoding at some future point.

    return ret;
}

// "encrypt_file" will also close the from and to file handles.
void encrypt_file( key *header, RSA *rsa, read_bufferfd &fromfd, write_bufferfd &tofd )
{
    const size_t key_size=RSA_size(rsa);

    /* Skip the header. We'll only write it out once the file itself is written */
    autofd::lseek(tofd, header_size(rsa), SEEK_SET);

    redir_pipe ipipe(8000);
    redir_fd redir_from(fromfd);
    process_ctl gzip_process( const_cast<char *>(FILENAME(gzip)), &redir_from, &ipipe, NULL,  "--rsyncable", NULL );

    // Run through gzip's output, and encrypt it
    const size_t block_size=header->block_size(); // Let's cache the block size
    auto_array<unsigned char> buffer(new unsigned char [block_size]);
    unsigned int i=0;
    int numread=1;
    bool new_block=true;

    read_bufferfd *readfd=new read_bufferfd(ipipe.get_read());
    while( (numread=readfd->read(buffer.get()+i, 1))!=0 ) {
        if( new_block ) {
            header->init_encrypt();
            new_block=false;
        }

        new_block=header->calc_boundry( buffer[i] );
        i+=numread;
        if( i>=block_size || new_block ) {
            header->encrypt_block( buffer.get(), i );
            tofd.write( buffer.get(), block_size );

            i=0;
        }
    }
    
    delete readfd;

    if( i>0 ) {
        // Still some leftover bytes to encrypt
        header->encrypt_block( buffer.get(), i );
        tofd.write( buffer.get(), block_size );
    }

    // Report how many bytes of last block are relevant.
    bzero( buffer.get(), block_size );
    buffer[0]=i;
    header->init_encrypt();
    header->encrypt_block( buffer.get(), 1 );
    tofd.write( buffer.get(), block_size );

    tofd.flush();

    // Wait for gzip to return, and check whether it succeeded
    int childstatus=gzip_process.wait();

    if( childstatus==0 ) {
        /* gzip was successful - write out the header, encrypted */
        autommap buffer( NULL, key_size, PROT_READ|PROT_WRITE, MAP_SHARED, tofd, 0 );
        encrypt_header( header, rsa, buffer.get_uc() );
    } else {
        throw rscerror("Error in running gzip");
    }

    tofd.clear();
}

// "decrypt_file" will also close the from and to file handles.
std::unique_ptr<key> decrypt_file( std::unique_ptr<key> header, RSA *prv, read_bufferfd &fromfd, write_bufferfd &tofd )
{
    if( header==NULL ) {
        /* Need to reconstruct the header from the encrypted file */
        header=decrypt_header( fromfd, prv );
    }

    /* If file does not contain a valid header - abort */
    if( header==NULL )
        throw rscerror("Couldn't extract encryption header");

    struct stat filestat;
    off_t currpos;

    filestat=fromfd.fstat();

    /* Skip the header */
    currpos=fromfd.lseek(header_size(prv), SEEK_SET);

    redir_pipe opipe;
    redir_fd redir_to(tofd);
    process_ctl gzip_process( const_cast<char *>(FILENAME(gzip)), &opipe, &redir_to, NULL, "-d", NULL );

    size_t numread;
    const size_t block_size=header->block_size();
    auto_array<unsigned char> buffer(new unsigned char [block_size]);
    bool done=false;
    bool new_block=true;
    write_bufferfd *writefd=new write_bufferfd(opipe.get_write());

    /* Read the file one AES_BLOCK_SIZE at a time, decrypt and write to the pipe */
    while( !done && (numread=fromfd.read( buffer.get(), block_size))!=0 ) {
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

        if( currpos>=filestat.st_size-static_cast<off_t>(block_size) ) {
            done=true;

            // Oops - file is not a whole multiple of block size
            if( currpos>filestat.st_size-static_cast<off_t>(block_size) )
                throw rscerror("Uneven file end");
        } else {
            writefd->write( buffer.get(), i );
        }

        // If this was not a full block, the remaining bytes should be zero
        for( ; i<block_size; ++i )
            if( buffer[i]!=0 )
                throw rscerror("Error in encrypted stream");
    }
    
    // The next block will tell us how many bytes of the last block should be written.
    auto_array<unsigned char> buffer2(new unsigned char [block_size]);
    if( fromfd.read( buffer2.get(), block_size)!=static_cast<ssize_t>(block_size) )
        throw rscerror("Unexcpeted end of file past sanity checks");

    header->init_encrypt();
    header->decrypt_block( buffer2.get(), block_size );
    for( unsigned int i=1; i<block_size; ++i )
        if( buffer2[i]!=0 )
            throw rscerror("Error in encrypted stream (trailer)");
    if( buffer2[0]>=block_size )
        throw rscerror("Error in encrypted stream (trailer 2)");

    if( buffer2[0]==0 )
        buffer2[0]=block_size;
    
    writefd->write( buffer.get(), buffer2[0] );
    writefd->flush();
    delete writefd;
    opipe.clear();

    int child_status=gzip_process.wait();

    if( child_status!=0 )
        throw rscerror("gunzip failed to run");

    fromfd.clear();
    
    return header;
}
