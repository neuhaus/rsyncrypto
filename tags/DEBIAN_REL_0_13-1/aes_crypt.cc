#include "rsyncrypto.h"
#include "aes_crypt.h"

aes_key::aes_key( uint16_t key_size,  uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist,
        const unsigned char *buffer ) : key(key_size, CYPHER_AES, sum_span, sum_mod, sum_min_dist),
                                        secret_key(new unsigned char[key_size])
{
    const aes_export *import=reinterpret_cast<const aes_export *>(buffer);

    memcpy( aes_header.iv, import->iv, sizeof(aes_header.iv) );
    memcpy( secret_key.get(), import->key, key_size );

    update_keys();
}

aes_key::aes_key( size_t keybits, uint32_t sum_span, uint32_t sum_mod, uint32_t sum_min_dist ) :
    key(keybits==0?32:(keybits+7)/8, CYPHER_AES, sum_span, sum_mod, sum_min_dist ),
    secret_key(new unsigned char[header.key_size])
{
    if( !RAND_bytes(aes_header.iv, sizeof( aes_header.iv )) ||
                !RAND_bytes(secret_key.get(), header.key_size) )
            throw rscerror("No random entropy for key and IV");

    update_keys();
}

aes_key::aes_key( const aes_key &that ) : key( that.header.key_size, CYPHER_AES, that.header.sum_span,
        that.header.sum_mod, that.header.sum_min_dist ), secret_key( new unsigned char [header.key_size] )
{
    memcpy( aes_header.iv, that.aes_header.iv, sizeof( aes_header.iv ) );
    memcpy( secret_key.get(), that.secret_key.get(), header.key_size );

    update_keys();
}

size_t aes_key::export_key( void *buffer ) const
{
    unsigned char *buff=static_cast<unsigned char *>(buffer);
    size_t length=key::export_key( buffer );
    
    memcpy( buff+length, aes_header.iv, sizeof( aes_header.iv ) );
    length+=sizeof( aes_header.iv );

    memcpy( buff+length, secret_key.get(), header.key_size );
    length+=header.key_size;

    return length;
}

key *aes_key::gen_pad_key() const
{
    std::auto_ptr<aes_key> ret(new aes_key(*this));

    for( unsigned int i=0; i<sizeof(aes_header.iv); ++i )
        ret->aes_header.iv[i]=~ret->aes_header.iv[i];

    return ret.release();
}

void aes_key::init_encrypt()
{
    key::init_encrypt();

    memcpy( cbc_base, aes_header.iv, sizeof(cbc_base) );
}

void aes_key::encrypt_block( unsigned char *data, size_t size )
{
    AES_cbc_encrypt( data, data, size, &encrypt_key, cbc_base, AES_ENCRYPT );
}

void aes_key::decrypt_block( unsigned char *data, size_t size )
{
    AES_cbc_encrypt( data, data, size, &decrypt_key, cbc_base, AES_DECRYPT );
}