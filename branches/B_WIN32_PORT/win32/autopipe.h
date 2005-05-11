// This is the Win32 version of this struct
#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

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
            ODS("CreatePipe Read: %08x, Write %08x\n", hReadPipe, hWritePipe );
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
    void clear_read()
    {
        input.clear();
    }
    void clear_write()
    {
        output.clear();
    }
    void clear()
    {
        clear_read();
        clear_write();
    }
};

#endif // _AUTOPIPE_H
