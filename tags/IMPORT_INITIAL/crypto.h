#ifndef CRYPTO_H
#define CRYPTO_H
#include <openssl/rsa.h>

enum CYPHER_TYPE { CYPHER_AES };

struct key_header;

struct key_header *gen_header(int key_length, enum CYPHER_TYPE cypher);
int encrypt_header( const struct key_header *header, RSA *rsa, unsigned char *to );
RSA *extract_public_key( const char *pem_filename );
int encrypt_file( const struct key_header *header, RSA *rsa, int fromfd, int tofd );

#endif /* CRYPTO_H */
