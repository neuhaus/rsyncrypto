#ifndef RSYNCRYPTO_H
#define RSYNCRYPTO_H

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <dirent.h>

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

class rscerror {
    std::string msg;
    std::string sysmsg;
    std::string param;
public:
    explicit rscerror( const char *msg_p ) : msg(msg_p)
    {
    }
    explicit rscerror( const char *msg_p, int error, const char *param_p=NULL ) : msg(msg_p),
                                                                                sysmsg(strerror(error)),
                                                                                param(param_p)
    {
    }

    std::string error() const {
        std::string ret(msg);
        if( param.length()!=0 )
            ret+="("+param+")";
        ret+=": "+sysmsg;

        return ret;
    }
};

struct startup_options {
    size_t keysize;
    uint32_t rollwin, rollmin, rollsens;
    bool fr, fk;
    const char *gzip;
    int verbosity;
    bool decrypt;
    bool archive;
    bool recurse;
    int trim;

    startup_options() : keysize(0), rollwin(0), rollmin(0), rollsens(0), fr(false), fk(false),
			gzip("gzip"), verbosity(0), decrypt(false), archive(true), recurse(false), trim(-1)
    {
    }
};

extern startup_options options;
extern std::ostream *report0, *report1, *report2;

#define EXCEPT_CLASS rscerror

#include "autoarray.h"
#include "autommap.h"
#include "autofd.h"

#endif // RSYNCRYPTO_H
