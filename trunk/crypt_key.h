#ifndef CRYPT_KEY_H
#define CRYPT_KEY_H

class key {
private:
    static const uint32_t VERSION_MAGIC_1=0xD657EA1Cul;
    static const uint32_t VERSION_MAGIC_CUR=VERSION_MAGIC_1;
protected:
    // Define the key header in it's export form
    struct ext_key_header {
        uint32_t version;
        uint16_t cypher;
        uint16_t key_size; /* key size IN BYTES! */
        uint32_t sum_span, sum_mod, sum_min_dist;
    } header;

    enum CYPHER_TYPES { CYPHER_AES };
    key(uint16_t key_size, CYPHER_TYPES cypher, uint32_t sum_span=256, uint32_t sum_mod=8192,
            uint32_t sum_min_dist=8192 )
    {
        header.version=VERSION_MAGIC_CUR;

        header.key_size=key_size;
        header.cypher=cypher;
        header.sum_span=sum_span;
        header.sum_mod=sum_mod;
        header.sum_min_dist=sum_min_dist;
    }
public:
    virtual ~key()
    {
    }

private:
public:
    virtual size_t exported_length() const
    {
        return sizeof(ext_key_header)+header.key_size+block_size();
    }
    virtual size_t block_size() const=0;
    static key *read_key( const unsigned char *buffer );
    static key *newkey( CYPHER_TYPES cypher=CYPHER_AES, size_t keybits=0, uint32_t sum_span=256,
            uint32_t sum_mod=8192, uint32_t sum_min_dist=8192 );
};

#endif // CRYPT_KEY_H
