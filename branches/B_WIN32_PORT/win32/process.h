// Win32 implementation
#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef PROCESS_H
#define PROCESS_H

class process_ctl {
    PROCESS_INFORMATION pInfo;
    autohandle hProcess;
    autohandle hThread;

    // Disable default copy ctor and operator=
    process_ctl( const process_ctl & );
    process_ctl &operator= ( const process_ctl & );
public:
    // The ... is substituted for further command line arguments, in execlp syntax
    process_ctl( const char *cmdline, file_t input, file_t output, ...);
};

#endif // PROCESS_H