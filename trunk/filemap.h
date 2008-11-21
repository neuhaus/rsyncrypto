#ifndef FILELIST_H
#define FILELIST_H

#ifndef FILE_H
#include "file.h"
#endif

class filemap;

typedef std::map<TSTRING, filemap> filemaptype;
typedef std::map<TSTRING, TSTRING> revfilemap;

class filemap {
    TSTRING plainname;
    TSTRING ciphername;

    char dirsep;

public:
    filemap() : dirsep('\0')
    {
    }
    static void fill_map( const TCHAR *list_filename, bool encrypt );
    static void write_map( const TCHAR *list_filename );

    // Helper functions for name conversions
    static TSTRING namecat_encrypt( const TCHAR *left, const TCHAR *right, mode_t mode );
    static TSTRING namecat_decrypt( const TCHAR *left, const TCHAR *right, mode_t mode );
    //static std::string create_combined_path( const char *left, const char *right );

    static void enc_file_delete( const TCHAR *source_dir, const TCHAR *dst_dir, const TCHAR *key_dir,
	    filemaptype::iterator &item, RSA *rsa_key );
private:
    static void nest_name( TSTRING &name );
};

extern filemaptype filelist;
extern revfilemap reversemap; // Cypher->plain mapping for encryption usage

// Helper functions for "dir" scan
typedef void (*encopfunc)( const TCHAR *source_dir, const TCHAR *dst_dir, const TCHAR *key_dir,
	filemaptype::iterator &item, RSA *rsa_key );
void virt_recurse_dir_enc( const TCHAR *encdir, const TCHAR *plaindir, const TCHAR *keydir,
	RSA *rsa_key, encopfunc op, const TCHAR *dir_sig_part );

// The file name by which the file list is stored inside the encrypted directory
static const TCHAR FILEMAPNAME[]=_T("filemap");

#endif
