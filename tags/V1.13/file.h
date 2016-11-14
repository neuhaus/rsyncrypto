#ifndef FILE_H
#define FILE_H

// Function types for processing files
typedef void (* encryptfunc)(const char *source, const char *dest, const char *key, RSA *rsa,
    const struct stat *stat );
typedef std::string(* namefunc)(const char *left, const char *right, mode_t mode );
typedef std::string(* prefix_func)(const char *left, const char *right, bool abs );

// A simple concatanation of both names
std::string name_concat( const char *left, const char *right, mode_t mode );

// Concat the names if "right" is not absolute, otherwise return only the right part
std::string cond_name_concat( const char *left, const char *right, bool abs );
// Always return the left part
std::string left_only_concat( const char *left, const char *right, bool abs );

void filelist_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname, prefix_func prefix_op, namefunc srcnameop, namefunc dstnameop,
        namefunc keynameop );
void dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname, namefunc dstname, namefunc keyname );
void file_encrypt( const char *source_file, const char *dst_file, const char *key_file,
        RSA *rsa_key, const struct stat *stat );
void file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key,
        const struct stat *stat );

#endif // FILE_H
