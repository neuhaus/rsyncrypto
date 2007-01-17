#ifndef CRYPT_KEY_H
#define CRYPT_KEY_H

class key {
private:
    static const uint32_t VERSION_MAGIC_1;
    static const uint32_t VERSION_MAGIC_CUR;

protected:
    // Define the key header in it's export form
    struct ext_key_header {
        uint32_t version;
        uint16_t cypher;
        uint16_t key_size; /* key size IN BYTES! */
        uint32_t sum_span, sum_mod, sum_min_dist;
    } header;

public:
    enum CYPHER_TYPES { CYPHER_AES };
private:
    auto_array<unsigned char> plaintext_buffer;
    size_t ptbuf_loc, ptbuf_count;
    unsigned long ptbuf_sum;
    bool ptbuf_sub;
    bool ptbuf_may_rotate;
protected:
    key(uint16_t key_size, CYPHER_TYPES cypher, uint32_t sum_span=256, uint32_t sum_mod=8192,
            uint32_t sum_min_dist=8192 ) : plaintext_buffer( new unsigned char [sum_span] )
    {
        header.version=VERSION_MAGIC_CUR;

        header.key_size=key_size;
        header.cypher=cypher;
        header.sum_span=sum_span;
        header.sum_mod=sum_mod;
        header.sum_min_dist=sum_min_dist;
    }

    virtual key *gen_pad_key() const=0;
public:
    virtual ~key()
    {
    }

private:
public:
    virtual size_t exported_length() const
    {
        return sizeof(ext_key_header);
    }
    // export_key returns the number of bytes the exported key took
    virtual size_t export_key( void *buffer ) const;
    virtual size_t block_size() const=0;
    static key *read_key( const unsigned char *buffer );
    static key *new_key( CYPHER_TYPES cypher=CYPHER_AES, size_t keybits=0, uint32_t sum_span=256,
            uint32_t sum_mod=8192, uint32_t sum_min_dist=8192 );

    // Encryption/decryption functions
    virtual void init_encrypt(); // Reset the IV values
    // Encrypt/decrypt in place. "size" is not guarenteed to work if bigger than block_size
    virtual void encrypt_block( unsigned char *data, size_t size )=0;
    virtual void decrypt_block( unsigned char *data, size_t size )=0;
    // Calculate whether we are on block boundry. If we are, we need to flush the plaintext and
    // reset IV AFTER the current byte.
    virtual bool calc_boundry( unsigned char data );

    // Fill a memory area in a random-predictable way, based on the IV
    virtual void pad_area( unsigned char *buffer, size_t size ) const;

    // Simple retriever methods
    uint16_t get_key_size() const
    {
        return header.key_size;
    }
    uint32_t get_sum_span() const
    {
        return header.sum_span;
    }
    uint32_t get_sum_mod() const
    {
        return header.sum_mod;
    }
    uint32_t get_sum_min_dist() const
    {
        return header.sum_min_dist;
    }
};

#endif // CRYPT_KEY_H
