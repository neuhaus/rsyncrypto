#ifndef RSYNCRYPTO_H
#define RSYNCRYPTO_H

#include <string>

#include "config.h"

#include "autoarray.h"
#include "autommap.h"
#include "autofd.h"

class rscerror {
    std::string msg;
public:
    explicit rscerror( const char *msg_p ) : msg(msg_p)
    {
    }
};

#endif // RSYNCRYPTO_H
