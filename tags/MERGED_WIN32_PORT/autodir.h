#if defined(_WIN32)
#include "win32/autodir.h"
#elif !defined(AUTODIR_H)
#define AUTODIR_H

// automap will auto-release mmaped areas
class autodir {
    DIR *dir;

    // Disable default copy constructor
    autodir( const autodir & );
    autodir &operator=( const autodir & );
public:
    explicit autodir( const char *dirname ) : dir(opendir(dirname))
    {
#if defined(EXCEPT_CLASS)
        if( dir==NULL )
            throw rscerror("opendir failed", errno, dirname);
#endif
    }
    ~autodir()
    {
        clear();
    }
    DIR *get() const
    {
        return dir;
    }

    void clear()
    {
        if( dir!=NULL ) {
            closedir( dir );
            dir=NULL;
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
