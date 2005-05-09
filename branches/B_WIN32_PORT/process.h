#ifndef PROCESS_H
#if defined(_WIN32)
#include "win32/process.h"
#else
#define PROCESS_H

#include "redir.h"

class process_ctl {
    int pid;

    // Disable default copy ctor and operator=
    process_ctl( const process_ctl & );
    process_ctl &operator= ( const process_ctl & );
public:
    process_ctl( const char *cmdline, redir *input, redir *output, redir *error, ... );

    int wait() const;
};
#endif // _WIN32
#endif // PROCESS_H
