#ifndef RSYNCRYPTO_H
#define RSYNCRYPTO_H

#include <string>

#include "config.h"

class rscerror {
    std::string msg;
public:
    explicit rscerror( const char *msg_p ) : msg(msg_p)
    {
    }
    explicit rscerror( int error ) : msg(strerror(error))
    {
    }
};

#define EXCEPT_CLASS rscerror

#include "autoarray.h"
#include "autommap.h"
#include "autofd.h"

#endif // RSYNCRYPTO_H
