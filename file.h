#ifndef FILE_H
#define FILE_H

// Function types for processing files
typedef void (* encryptfunc)(const TCHAR *source, const TCHAR *dest, const TCHAR *key, RSA *rsa,
    const struct stat *stat );
typedef TSTRING(* namefunc)(const TCHAR *left, const TCHAR *right, mode_t mode );

// A simple concatanation of both names
TSTRING name_concat( const TCHAR *left, const TCHAR *right, mode_t mode );

void filelist_encrypt( const TCHAR *src_dir, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
        encryptfunc op, const TCHAR *opname, namefunc srcnameop, namefunc dstnameop, namefunc keynameop );
void dir_encrypt( const TCHAR *src_dir, const TCHAR *dst_dir, const TCHAR *key_dir, RSA *rsa_key,
        encryptfunc op, const TCHAR *opname, namefunc dstname, namefunc keyname );
void file_encrypt( const TCHAR *source_file, const TCHAR *dst_file, const TCHAR *key_file,
        RSA *rsa_key, const struct stat *stat );
void file_decrypt( const TCHAR *src_file, const TCHAR *dst_file, const TCHAR *key_file, RSA *rsa_key,
        const struct stat *stat );

#endif // FILE_H
