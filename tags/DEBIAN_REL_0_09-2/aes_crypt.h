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
    aes_key( const aes_key &that );

    unsigned char cbc_base[AES_BLOCK_SIZE];
    AES_KEY encrypt_key, decrypt_key;

    void update_keys()
    {
        AES_set_decrypt_key( secret_key.get(), header.key_size*8, &decrypt_key );
        AES_set_encrypt_key( secret_key.get(), header.key_size*8, &encrypt_key );
    }
protected:
    virtual key *gen_pad_key() const;
public:
    aes_key( uint16_t key_size,  uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist,
            const unsigned char *buffer );
    aes_key( size_t keybits, uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist );
    virtual size_t block_size() const
    {
        return AES_BLOCK_SIZE;
    }
    virtual size_t exported_length() const
    {
        return key::exported_length()+header.key_size+block_size();
    }
    virtual size_t export_key( void *buffer ) const;

    // Encryption/decryption functions
    virtual void init_encrypt(); // Reset the IV values
    // Encrypt/decrypt in place. "size" is not guarenteed to work if bigger than block_size
    virtual void encrypt_block( unsigned char *data, size_t size );
    virtual void decrypt_block( unsigned char *data, size_t size );
};

#endif // AES_CRYPT_H
