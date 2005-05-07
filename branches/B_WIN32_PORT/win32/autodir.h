// Win32 implementation

#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#if !defined(AUTODIR_H)
#define AUTODIR_H

class autodir {
    HANDLE h_dirscan;
    WIN32_FIND_DATA finddata;

    // Disable default copy constructor
    autodir( const autodir & );
    autodir &operator=( const autodir & );
public:
    explicit autodir( const char *dirname )
    {
        h_dirscan=FindFirstFile((std::string(dirname)+"\\*").c_str(), &finddata );
#if defined(EXCEPT_CLASS)
        if( h_dirscan==INVALID_HANDLE_VALUE )
            throw rscerror("opendir failed", GetLastError(), dirname);
#endif
    }
    ~autodir()
    {
        clear();
    }

    void clear()
    {
        if( h_dirscan!=INVALID_HANDLE_VALUE ) {
            FindClose( h_dirscan );
            h_dirscan=INVALID_HANDLE_VALUE;
        }
    }

    struct dirent *read()
    {
        return ::readdir(dir);
    }
    void rewind()
    {
        ::rewinddir(dir);
    }
    void seek( off_t offset )
    {
        seekdir( dir, offset );
    }
    off_t telldir()
    {
        return ::telldir( dir );
    }
};

#endif // AUTODIR_H
