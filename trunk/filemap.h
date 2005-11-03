#ifndef FILELIST_H
#define FILELIST_H

#ifndef FILE_H
#include "file.h"
#endif

class filemap;

typedef std::map<std::string, filemap> filemaptype;
typedef std::map<std::string, std::string> revfilemap;

class filemap {
    std::string plainname;
    std::string ciphername;

    char dirsep;

public:
    filemap() : dirsep('\0')
    {
    }
    static void fill_map( const char *list_filename, bool encrypt );
    static void write_map( const char *list_filename );

    // Helper functions for name conversions
    static std::string namecat_encrypt( const char *left, const char *right, mode_t mode );
    static std::string namecat_decrypt( const char *left, const char *right, mode_t mode );
    //static std::string create_combined_path( const char *left, const char *right );

    static void enc_file_delete( const char *source_dir, const char *dst_dir, const char *key_dir,
	    filemaptype::iterator &item, RSA *rsa_key );
private:
    static void nest_name( std::string &name );
};

extern filemaptype filelist;
extern revfilemap reversemap; // Cypher->plain mapping for encryption usage

// Helper functions for "dir" scan
typedef void (*encopfunc)( const char *source_dir, const char *dst_dir, const char *key_dir,
	filemaptype::iterator &item, RSA *rsa_key );
void virt_recurse_dir_enc( const char *encdir, const char *plaindir, const char *keydir,
	RSA *rsa_key, encopfunc op, const char *dir_sig_part );

// The file name by which the file list is stored inside the encrypted directory
static const char FILEMAPNAME[]="filemap";

#endif
