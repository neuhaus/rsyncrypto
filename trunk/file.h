#ifndef FILE_H
#define FILE_H

// Function types for processing files
typedef void (* encryptfunc)(const char *source, const char *dest, const char *key, RSA *rsa);
typedef std::string(* namefunc)(const char *left, const char *right, mode_t mode );

// A simple concatanation of both names
std::string name_concat( const char *left, const char *right, mode_t mode );

void filelist_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname );
void dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname, namefunc dstname, namefunc keyname );
void file_encrypt( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key );
void file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key);

int calc_trim( const char *path, int trim_count );

#endif // FILE_H
