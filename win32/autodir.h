// Win32 implementation

#if !defined(_WIN32)
#error Win32 only header included from non-Win32 build environment
#endif

#if !defined(AUTODIR_H)
#define AUTODIR_H

struct dirent {
    char d_name[MAX_PATH];
};

class autodir {
    HANDLE h_dirscan;
    WIN32_FIND_DATA finddata;
    dirent posixdir;
    bool eof;
    bool utf8;

    // Disable default copy constructor
    autodir( const autodir & );
    autodir &operator=( const autodir & );

    // Convert wide find data to UTF8
    void data_w2a( const WIN32_FIND_DATAW *data );
public:
    explicit autodir( const char *dirname, bool _utf8=false );
    ~autodir()
    {
        clear();
    }

    void clear()
    {
        if( h_dirscan!=INVALID_HANDLE_VALUE ) {
            FindClose( h_dirscan );
            h_dirscan=INVALID_HANDLE_VALUE;
            eof=false;
        }
    }

    struct dirent *read();
    
    /*
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
    */
};

#endif // AUTODIR_H
