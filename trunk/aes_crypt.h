#ifndef AES_CRYPT_H
#define AES_CRYPT_H

#include "crypt_key.h"

class aes_key : public key {
public:
    aes_key( uint16_t key_size,  uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist,
            const unsigned char *buffer ) : key(key_size, CYPHER_AES, sum_span, sum_mod, sum_min_dist)
    {
    }
};

#endif // AES_CRYPT_H
