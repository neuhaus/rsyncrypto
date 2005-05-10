// Win32 implementation

#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#if !defined(WIN32_REDIR_H)
#define WIN32_REDIR_H

struct win32_redir_opaq {
    STARTUPINFO *si;
    autohandle handle;
};

#endif // WIN32_REDIR_H