#ifndef FILE_H
#define FILE_H

typedef int (* encryptfunc)(const char *source, const char *dest, const char *key, RSA *rsa);

int dir_encrypt( const char *src_dir, const char *dst_dir, const char *key_dir, RSA *rsa_key,
        encryptfunc op, const char *opname );
int file_encrypt( const char *source_file, const char *dst_file, const char *key_file, RSA *rsa_key );
int file_decrypt( const char *src_file, const char *dst_file, const char *key_file, RSA *rsa_key);

#endif // FILE_H
