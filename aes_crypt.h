#ifndef AES_CRYPT_H
#define AES_CRYPT_H

#include "crypt_key.h"
#include "autoarray.h"
#include <openssl/aes.h>

class aes_key : public key {
    struct aes_export {
        unsigned char iv[AES_BLOCK_SIZE];
        unsigned char key[0];
    } aes_header;
    auto_array<unsigned char> secret_key;
public:
    aes_key( uint16_t key_size,  uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist,
            const unsigned char *buffer );
    aes_key( size_t keybits, uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist );
    virtual size_t block_size() const
    {
        return AES_BLOCK_SIZE;
    }
};

#endif // AES_CRYPT_H
