#ifndef RSYNCRYPTO_H
#define RSYNCRYPTO_H

#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>

#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <memory>
#include <iostream>
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

    const char *error() const {
        return msg.c_str();
    }
};

struct startup_options {
    size_t keysize;
    uint32_t rollwin, rollmin, rollsens;
    bool fr, fk;
    const char *gzip;

    startup_options() : keysize(0), rollwin(256), rollmin(8192), rollsens(8192), fr(false), fk(false),
			gzip("gzip")
    {
    }
};

extern startup_options options;

#define EXCEPT_CLASS rscerror

#include "autoarray.h"
#include "autommap.h"
#include "autofd.h"

#endif // RSYNCRYPTO_H
