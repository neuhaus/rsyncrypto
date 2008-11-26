// Win32 implementation
#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef PROCESS_H
#define PROCESS_H

#include "../redir.h"

class process_ctl {
    PROCESS_INFORMATION pInfo;
    autohandle hProcess;
    autohandle hThread;

    // Disable default copy ctor and operator=
    process_ctl( const process_ctl & );
    process_ctl &operator= ( const process_ctl & );
public:
    // The ... is substituted for further command line arguments, in execlp syntax
    process_ctl( TCHAR *cmdline, redir *input, redir *output, redir *error, ... );

    int wait() const;
};

#endif // PROCESS_H
