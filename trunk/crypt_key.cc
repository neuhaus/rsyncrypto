#include <memory>
#include <stdint.h>
#include <netinet/in.h>
#include "rsyncrypto.h"
#include "crypt_key.h"
#include "aes_crypt.h"

key *key::read_key( const unsigned char *buffer )
{
    const ext_key_header *buff=reinterpret_cast<const ext_key_header *>(buffer);

    if( buff->version!=htonl(VERSION_MAGIC_1) )
        throw rscerror("Invalid version magic");

    std::auto_ptr<key> ret;
    
    switch( static_cast<CYPHER_TYPES>(buff->cypher) )
    {
    case CYPHER_AES:
        ret=std::auto_ptr<key>(new aes_key( ntohs(buff->key_size), ntohl(buff->sum_span), ntohl(buff->sum_mod),
                ntohl(buff->sum_min_dist), buffer+sizeof(*buff) ));
        break;
    default:
        throw rscerror("Invalid block cypher");
    }

    return ret.release();
}

key *key::newkey( CYPHER_TYPES cypher, size_t keybits, uint32_t sum_span, uint32_t sum_mod,
        uint32_t sum_min_dist )
{
    std::auto_ptr<key> ret;

    switch( static_cast<CYPHER_TYPES>(cypher) ) {
    case CYPHER_AES:
        ret=std::auto_ptr<key>(new aes_key( keybits, sum_span, sum_mod, sum_min_dist ));
        break;
    default:
        throw rscerror("Invalid block cypher");
    }

    return ret.release();
}

size_t key::export_key( void *buffer ) const
{
    struct ext_key_header *export_buff=static_cast<ext_key_header *>(buffer);

    bzero(export_buff, sizeof(*export_buff));
    export_buff.version=htonl(header.version);
    export_buff.cypher=htons(header.cypher);
    export_buff.key_size=htons(header.key_size);
    export_buff.sum_span=htonl(header.sum_span);
    export_buff.sum_mod=htonl(header.sum_mod);
    export_buff.sum_min_dist=htonl(header.sum_min_dist);

    return key::exported_length();
}
