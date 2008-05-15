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

    // Disable default copy constructor
    autodir( const autodir & );
    autodir &operator=( const autodir & );
public:
    explicit autodir( const char *dirname ) : eof(false)
    {
        h_dirscan=FindFirstFile((std::string(dirname)+"\\*").c_str(), &finddata );
#if defined(EXCEPT_CLASS)
        if( h_dirscan==INVALID_HANDLE_VALUE )
            throw rscerror("opendir failed", Error2errno(GetLastError()), dirname);
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
            eof=false;
        }
    }

    struct dirent *read()
    {
        if( !eof ) {
            strcpy_s(posixdir.d_name, finddata.cFileName);
            if( !FindNextFile(h_dirscan, &finddata) ) {
                eof=true;
                DWORD error=GetLastError();
                if( error!=ERROR_NO_MORE_FILES ) {
                    throw rscerror("Error getting directory listing", Error2errno(error));
                }
            }
            return &posixdir;
        } else {
            return NULL;
        }
    }
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
