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
