// This is the Win32 version of this struct

#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#ifndef AUTOHANDLE_H
#define AUTOHANDLE_H

// automap will auto-release mmaped areas
class autohandle {
    HANDLE handle;
    mutable bool owner;

public:
    explicit autohandle( HANDLE handle_p=NULL ) : handle(handle_p),
        owner(valid())
    {
    }
    autohandle( const autohandle &that ) : handle(that.release()), owner(true)
    {
    }
    ~autohandle()
    {
        clear();
    }
    HANDLE get() const
    {
        return handle;
    }
    operator HANDLE() const
    {
        return get();
    }
    autohandle &operator=( const autohandle &that )
    {
        if( handle!=that.handle ) {
            clear();
            owner=that.owner;
            if( owner ) {
                handle=that.release();
            } else {
                handle=that.get();
            }
        }

        return *this;
    }
    void clear()
    {
        if( owner ) {
            CloseHandle( handle);
            handle=NULL;
            owner=false;
        }
    }
    HANDLE release() const
    {
#if defined(EXCEPT_CLASS)
        if( !owner )
            throw EXCEPT_CLASS("Releasing non-owner handle");
#endif

        owner=false;

        return handle;
    }
    bool valid() const
    {
        return handle!=NULL && handle!=INVALID_HANDLE_VALUE;
    }

    // WIN32 extensions
    autohandle Duplicate(bool inheritable) const
    {
        HANDLE hProcess=GetCurrentProcess();
        HANDLE hNewHandle=NULL;
        if( !DuplicateHandle(hProcess, handle, hProcess, &hNewHandle, 0, inheritable,
            DUPLICATE_SAME_ACCESS ) ) {
#if defined(EXCEPT_CLASS)
            throw EXCEPT_CLASS("Error duplicating handle", GetLastError());
#endif
        }

        return autohandle(hNewHandle);
    }
};

#endif // AUTOHANDLE_H
