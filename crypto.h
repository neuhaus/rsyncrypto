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
 * In addition, as a special exception, the rsyncrypto authors give permission
 * to link the code of this program with the OpenSSL library (or with modified
 * versions of OpenSSL that use the same license as OpenSSL), and distribute
 * linked combinations including the two. You must obey the GNU General Public
 * License in all respects for all of the code used other than OpenSSL. If you
 * modify this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * The project's homepage is at http://rsyncrypto.lingnu.com
 */

#ifndef CRYPTO_H
#define CRYPTO_H
#include <openssl/rsa.h>
#include "crypt_key.h"
#include "bufferfd.h"

enum CYPHER_TYPE { CYPHER_AES };

//class key_header;

//struct key_header *gen_header(int key_length, enum CYPHER_TYPE cypher);
std::unique_ptr<key> read_header( const autofd &headfd );
void write_header( const char *filename, const key *head );
size_t header_size( const RSA *rsa );
void encrypt_header( const struct key_header *header, RSA *rsa, unsigned char *to );
RSA *extract_public_key( const char *pem_filename );
RSA *extract_private_key( const char *key_filename );
void encrypt_file( key *header, RSA *rsa, read_bufferfd &fromfd, write_bufferfd &tofd );
std::unique_ptr<key> decrypt_file( std::unique_ptr<key> header, RSA *prv, read_bufferfd &fromfd, write_bufferfd &tofd );

#endif /* CRYPTO_H */
