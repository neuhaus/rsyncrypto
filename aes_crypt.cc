#include <stdint.h>

#include <openssl/rand.h>

#include "rsyncrypto.h"
#include "aes_crypt.h"

aes_key::aes_key( uint16_t key_size,  uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist,
        const unsigned char *buffer ) : key(key_size, CYPHER_AES, sum_span, sum_mod, sum_min_dist),
                                        secret_key(new unsigned char[key_size])
{
    const aes_export *import=reinterpret_cast<const aes_export *>(buffer);

    memcpy( aes_header.iv, import->iv, sizeof(aes_header.iv) );
    memcpy( secret_key.get(), import->key, key_size );
}

aes_key::aes_key( size_t keybits, uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist ) :
    key((keybits+7)/8, CYPHER_AES, sum_span, sum_mod, sum_min_dist ),
    secret_key(new unsigned char[header.key_size])
{
    if( !RAND_bytes(aes_header.iv, sizeof( aes_header.iv )) ||
                !RAND_bytes(secret_key.get(), header.key_size) )
            throw rscerror("No random entropy for key and IV");
}
size_t aes_key::export_key( void *buffer ) const
{
    unsigned char *buff=static_cast<unsigned char *>(buffer);
    size_t length=key::exporte_key( buffer );
    
    memcpy( buff+length, aes_header.iv, sizeof( aes_header.iv ) );
    length+=sizeof( aes_header.iv );

    memcpy( buff+length, secret_key.get(), header.key_size );
    length+=header.key_size;

    return length;
}
