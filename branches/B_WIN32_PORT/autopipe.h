// This is the Win32 version of this struct

#if defined(_WIN32)
#include "win32/autopipe.h"
#else
#ifndef _AUTOPIPE_H
#define _AUTOPIPE_H

class autopipe {
    autofd input, output;
public:
    explicit autopipe(DWORD pipe_size=4096) : input(INVALID_HANDLE_VALUE),
        output(INVALID_HANDLE_VALUE)
    {
        HANDLE hReadPipe, hWritePipe;

        if( CreatePipe(&hReadPipe, &hWritePipe, NULL, pipe_size ) ) {
            input=autofd(hReadPipe);
            output=autofd(hWritePipe);
        } else {
#if defined(EXCEPT_CLASS)
            throw EXCEPT_CLASS("Couldn't create pipe", GetLastError() );
#endif
        }
    }
    // Default copy constructor and operator= do exactly what we want.
    //autopipe( const autopipe &that )
    //autopipe &operator=( const autopipe &that )
    // As does default dtor
    //~autopipe()
    autofd &get_read()
    {
        return input;
    }
    const autofd &get_read() const
    {
        return input;
    }
    autofd &get_write()
    {
        return output;
    }
    const autofd &get_write() const
    {
        return output;
    }
};

#endif // _AUTOPIPE_H
#endif // _WIN32