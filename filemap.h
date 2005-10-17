#ifndef FILELIST_H
#define FILELIST_H

#ifndef FILE_H
#include "file.h"
#endif

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

private:
    static void nest_name( std::string &name );
};

typedef std::map<std::string, filemap> filemaptype;
extern filemaptype filelist;
typedef std::map<std::string, std::string> revfilemap;
extern revfilemap reversemap; // Cypher->plain mapping for encryption usage

// The file name by which the file list is stored inside the encrypted directory
static const char FILEMAPNAME[]="filemap";

#endif
