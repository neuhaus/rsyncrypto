// This is the Win32 version of this struct

#if defined(_WIN32)
#include "win32/autopipe.h"
#else
#ifndef _AUTOPIPE_H
#define _AUTOPIPE_H

class autopipe {
    autofd input, output;
public:
    explicit autopipe(size_t pipe_size=4096) : input(-1), output(-1)
    {
        int fd[2];

        if( pipe(fd)==0 ) {
            input=autofd(fd[0]);
            output=autofd(fd[1]);
        } else {
#if defined(EXCEPT_CLASS)
            throw EXCEPT_CLASS("Couldn't create pipe", errno );
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
    void clear()
    {
        input.clear();
        output.clear();
    }
};

#endif // _AUTOPIPE_H
#endif // _WIN32
