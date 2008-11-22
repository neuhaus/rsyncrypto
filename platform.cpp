#include <precomp.h>
#include "platform.h"

// Unix only has ANSI support
std::string a2t( const char *str, size_t len, bool utf8 )
{
    if( len==0 )
        return std::string( str );
    else
        return std::string( str, len );
}

std::string t2a( const char *str, size_t len, bool utf8 )
{
    return a2t( str, len, utf8 );
}

