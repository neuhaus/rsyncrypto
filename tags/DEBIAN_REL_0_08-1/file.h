#ifndef FILE_H
#define FILE_H

typedef void (* encryptfunc)(const char *source, const char *dest, const char *key, RSA *rsa);

void dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname );
void file_encrypt( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key );
void file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key);

#endif // FILE_H
