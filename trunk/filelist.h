#ifndef FILELIST_H
#define FILELIST_H

#ifndef FILE_H
#include "file.h"
#endif

class metadata {
    std::string plainname;
    std::string ciphername;

    char dirsep;
#ifdef __unix
    uid_t uid;
    gid_t gid;
    mode_t mode; 
#endif

    static size_t readchunk( const autommap &map, size_t offset, bool encrypt );

    bool metadata::readblock( const autommap &map, size_t offset, size_t *block_size,
            std::set<uint16_t> &blocks );
public:
    metadata() : dirsep('\0')
#ifdef __unix
                 ,uid(~(0u)), gid(~(0u)), mode(0)
#endif
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

typedef std::map<std::string, metadata> filelistmaptype;
extern filelistmaptype filelist;
typedef std::map<std::string, std::string> revlistmap;
extern revlistmap reversemap; // Cypher->plain mapping for encryption usage

// The file name by which the file list is stored inside the encrypted directory
static const char FILELISTNAME[]="filelist";

#endif