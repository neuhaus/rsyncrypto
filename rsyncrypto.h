#ifndef RSYNCRYPTO_H
#define RSYNCRYPTO_H

#include "rcserror.h"

struct startup_options {
    struct arg_lit *help, *del, *delkey, *fr, *fk, *noarch, *version;
    struct arg_file *filelist;
    struct arg_lit *decrypt, *verbosity, *recurse, *changed, *risky_writes;
    struct arg_int *keysize, *rollwin, *rollmin, *rollsens, *trim, *nenest;
    struct arg_int *noatime, *mod_win;
    struct arg_file *gzip;
    struct arg_file *src, *dst, *key, *master, *nameenc, *export_changes;
    struct arg_rem *rem1;
    struct arg_end *end;

    void *argtable[29
#if HAVE_NOATIME
        +1
#endif
        ];

    startup_options()
    {
        int i=0;
        argtable[i++]=src=arg_file1( NULL, NULL, _T("<src>"), _T("Source file or (if -r or --filelist is given) directory") );
        argtable[i++]=dst=arg_file1( NULL, NULL, _T("<dst>"), _T("Destination file or directory") );
        argtable[i++]=key=arg_file1( NULL, NULL, _T("<key>"), _T("Keys file or directory") );
        argtable[i++]=master=arg_file1( NULL, NULL, _T("<master key>"),
                _T("Master key (public key certificate or private key)") );
        argtable[i++]=help=arg_lit0( _T("h"), _T("help"), _T("Display this page."));
        argtable[i++]=version=arg_lit0( _T("V"), _T("version"), _T("Display the rsyncrypto version."));
        argtable[i++]=verbosity=arg_litn( _T("v"), _T("verbose"), 0, 5,
                _T("Produce more verbose output. Specify repeatedly for more verbosity"));
        argtable[i++]=decrypt=arg_lit0( _T("d"), _T("decrypt"), _T("Decrypt"));
        argtable[i++]=recurse=arg_lit0( _T("r"), _T("recurse"),
                _T("<src> <dst> and <keys> are directory names, and are processed recursively"));
        argtable[i++]=changed=arg_lit0( _T("c"), _T("changed"), _T("Only encrypt changed files. Requires -r or --filelist"));
        argtable[i++]=mod_win=arg_int0( NULL, _T("modify-window"), _T("<n>"), _T("compare mod-times with reduced accuracy") );
        argtable[i++]=export_changes=arg_file0( NULL, _T("export-changes"), _T("log_file"), _T("Write list of affected files to a log file") );
        argtable[i++]=nameenc=arg_file0( _T("n"), _T("name-encrypt"), _T("translation_file"), _T("Encrypt file names"));
        argtable[i++]=nenest=arg_int0( NULL, _T("ne-nesting"), _T("<n>"), _T("set the hash directory tree depth when encrypting file names") );
        argtable[i++]=trim=arg_int0( NULL, _T("trim"), _T("<n>"),
                _T("Number of directory entries to trim from the begining of the path. Default 1"));
        argtable[i++]=del=arg_lit0( NULL, _T("delete"), _T("Delete files under <dst> not under <src>. Requires -r"));
        argtable[i++]=delkey=arg_lit0( NULL, _T("delete-keys"), _T("Delete also the keys. Implies --delete"));
        argtable[i++]=filelist=arg_file0( NULL, _T("filelist"), _T("<file>"),
                _T("<file> is a list of file and directory names to process. \"-\" means read from stdin."));
        argtable[i++]=risky_writes=arg_lit0( NULL, _T("risky-writes"), _T("Write files in place - do not do safe replacement") );
        noatime=arg_int0( NULL, _T("noatime"), _T("<n>"), _T("Level of O_NOATIME use") );
#if HAVE_NOATIME
        argtable[i++]=noatime;
#endif
        argtable[i++]=keysize=arg_int0( _T("b"), _T("keybits"), _T("<n>"), _T("Size of key to create. Encryption only"));
        argtable[i++]=fr=arg_lit0( NULL, _T("fr"),
                _T("Force new rollover parameters, even if previous encryption used a different setting."));
        argtable[i++]=fk=arg_lit0( NULL, _T("fk"),
                _T("Force a new key size, even if previous encryption used a different setting."));
        argtable[i++]=noarch=arg_lit0( NULL, _T("no-archive-mode"), _T("Do not try to preserve timestamps"));
        argtable[i++]=gzip=arg_file0( NULL, _T("gzip"), _T("<file>"),
                _T("Path to gzip-like program to use. Must accept a --rsyncable command option"));
        argtable[i++]=rem1=arg_rem( _T("Advanced options:"), _T("") );
        argtable[i++]=rollwin=arg_int0( NULL, _T("roll-win"), _T("<n>"), _T("Rollover window size. Default is 8192 byte"));
        argtable[i++]=rollmin=arg_int0( NULL, _T("roll-min"), _T("<n>"),
                _T("Minimal number of guaranteed non-rolled bytes. Default 8192"));
        argtable[i++]=rollsens=arg_int0( NULL, _T("roll-sensitivity"), _T("<n>"),
                _T("How sensitive are we to cutting a block. Default is \"roll-win\""));
        argtable[i++]=end=arg_end(2);

        // If this assert fails, you forgot to fix the size of the "Argtable" array.
        assert(i==sizeof(argtable)/sizeof(argtable[0])); 

        if( arg_nullcheck(argtable)==0 ) {
            // Fill in default values
            keysize->ival[0]=0;
            trim->ival[0]=1;
            rollwin->ival[0]=8192;
            rollmin->ival[0]=8192;
            gzip->filename[0]=_T("gzip");
            nenest->ival[0]=0;
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
extern std::auto_ptr<std::ostream> changes_log;

#define EXCEPT_CLASS rscerror

// Add a constant suffix to files while they are being created
#define CREATE_SUFFIX ".rsyncrypto_tmp"

#if defined(_WIN32)

static inline void ODS(const char *format, ... )
{
#ifdef DEBUG
    char buffer[500];

    va_list args;
    va_start(args, format);

    _vsnprintf(buffer, sizeof(buffer), format, args );
    OutputDebugString(buffer);
#endif
}

#include "win32/types.h"
#endif

#endif // RSYNCRYPTO_H
