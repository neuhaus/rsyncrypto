#ifndef RSYNCRYPTO_H
#define RSYNCRYPTO_H

#ifndef _WIN32
#include "config.h"
#define DIRSEP_C '/'
#define DIRSEP_S "/"
#else
#define PACKAGE_NAME "rsyncrypto"
#define DIRSEP_C '\\'
#define DIRSEP_S "\\"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__unix__)
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#elif defined(_WIN32)
#define STRICT
#include <windows.h>
#include <sys/utime.h>
#else
#error Unsupported platform
#endif

#include <stdlib.h>
#include <stdarg.h>

#include <assert.h>
#include <string.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdlib.h>

#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <memory>
#include <iostream>
#include <string>

#if HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#define HASH_MAP 1
#else
#include <map>
#endif

#include <argtable2.h>

#if !HAVE_LSTAT
// Some platforms don't support "lstat" and links
#define lstat stat
#endif

class rscerror {
    std::string msg;
    std::string sysmsg;
    std::string param;
    int errnum;
public:
    explicit rscerror( const char *msg_p ) : msg(msg_p)
    {
    }
    explicit rscerror( const char *msg_p, int error, const char *param_p="" ) : msg(msg_p),
                                                                                sysmsg(strerror(error)),
                                                                                param(param_p),
                                                                                errnum(error)
    {
    }

    std::string error() const {
        std::string ret(msg);
        if( param.length()!=0 )
            ret+="("+param+")";
        ret+=": "+sysmsg;

        return ret;
    }
    int errornum() const {
        return errnum;
    }
};

struct startup_options {
    struct arg_lit *help, *del, *delkey, *filelist, *fr, *fk, *noarch;
    struct arg_lit *decrypt, *verbosity, *recurse, *changed;
    struct arg_int *keysize, *rollwin, *rollmin, *rollsens, *trim;
    struct arg_file *gzip;
    struct arg_file *src, *dst, *key, *master, *metaenc;
    struct arg_rem *rem1;
    struct arg_end *end;

    void *argtable[24];

    startup_options()
    {
        int i=0;
        argtable[i++]=src=arg_file1( NULL, NULL, "<src>", "Source file or directory (or filelist)" );
        argtable[i++]=dst=arg_file1( NULL, NULL, "<dst>", "Destination file or directory" );
        argtable[i++]=key=arg_file1( NULL, NULL, "<key>", "Keys file or directory" );
        argtable[i++]=master=arg_file1( NULL, NULL, "<master key>",
                "Master key (public key certificate or private key)" );
        argtable[i++]=help=arg_lit0( "h", "help", "Display this page.");
        argtable[i++]=verbosity=arg_litn( "v", "verbose", 0, 5,
                "Produce more verbose output. Specify repeatedly for more verbosity");
        argtable[i++]=decrypt=arg_lit0( "d", "decrypt", "Decrypt");
        argtable[i++]=recurse=arg_lit0( "r", "recurse",
                "<src> <dst> and <keys> are directory names, and are processed recursively");
        argtable[i++]=changed=arg_lit0( "c", "changed", "Only encrypt changed files. Requires -r");
        argtable[i++]=metaenc=arg_file0( "m", "meta-encrypt", "translation_file", "Encrypt meta data (file names, permissions)");
        argtable[i++]=trim=arg_int0( NULL, "trim", "<n>",
                "Number of directory entries to trim from the begining of the path. Default 1");
        argtable[i++]=del=arg_lit0( NULL, "delete", "Delete files under <dst> not under <src>. Requires -r");
        argtable[i++]=delkey=arg_lit0( NULL, "delete-keys", "Delete also the keys. Implies --delete");
        argtable[i++]=filelist=arg_lit0( NULL, "filelist",
                "<src> is a list of file and directory names to process. \"-\" means read from stdin.");
        argtable[i++]=keysize=arg_int0( "b", "keybits", "<n>", "Size of key to create. Encryption only");
        argtable[i++]=fr=arg_lit0( NULL, "fr",
                "Force new rollover parameters, even if previous encryption used a different setting.");
        argtable[i++]=fk=arg_lit0( NULL, "fk",
                "Force a new key size, even if previous encryption used a different setting.");
        argtable[i++]=noarch=arg_lit0( NULL, "no-archive-mode", "Do not try to preserve timestamps");
        argtable[i++]=gzip=arg_file0( NULL, "gzip", "<file>",
                "Path to gzip-like program to use. Must accept a --rsyncable command option");
	argtable[i++]=rem1=arg_rem( "Advanced options:", "" );
        argtable[i++]=rollwin=arg_int0( NULL, "roll-win", "<n>", "Rollover window size. Default is 8192 byte");
        argtable[i++]=rollmin=arg_int0( NULL, "roll-min", "<n>",
                "Minimal number of guaranteed non-rolled bytes. Default 8192");
        argtable[i++]=rollsens=arg_int0( NULL, "roll-sensitivity", "<n>",
                "How sensitive are we to cutting a block. Default is \"roll-wine\"");
        argtable[i++]=end=arg_end(2);

	// If this assert fails, you forgot to fix the size of the "Argtable" array.
        assert(i==sizeof(argtable)/sizeof(argtable[0])); 

        if( arg_nullcheck(argtable)==0 ) {
            // Fill in default values
            keysize->ival[0]=0;
            trim->ival[0]=1;
            rollwin->ival[0]=8192;
            rollmin->ival[0]=8192;
            gzip->filename[0]="gzip";
        }
    }

    ~startup_options()
    {
        arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    }
};

extern startup_options options;
#define EXISTS(arg) (options.arg->count>0)
#define VAL(arg) (options.arg->ival[0])
#define FILENAME(arg) (options.arg->filename[0])
#define ARG(arg) (*(options.arg))
#define VERBOSE(val) (ARG(verbosity).count>=(val))
extern std::ostream *report0, *report1, *report2, *report3;

#define EXCEPT_CLASS rscerror

#include "autoarray.h"
#if defined(__unix__)
#include "autofd.h"
#include "autommap.h"
#elif defined(_WIN32)
static inline ODS(const char *format, ... )
{
    char buffer[500];

    va_list args;
    va_start(args, format);

    _vsnprintf(buffer, sizeof(buffer), format, args );
    OutputDebugString(buffer);
}

#include "win32/types.h"
#include "win32/autofd.h"
#include "win32/autommap.h"
#else
#error Unsupported platform
#endif

#endif // RSYNCRYPTO_H
