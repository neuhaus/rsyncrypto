/*
 * rsyncrypto - an rsync friendly encryption
 * Copyright (C) 2005 Shachar Shemesh for Lingnu Open Source Consulting ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * In addition, as a special exception, the rsyncrypto authors give permission
 * to link the code of this program with the OpenSSL library (or with modified
 * versions of OpenSSL that use the same license as OpenSSL), and distribute
 * linked combinations including the two. You must obey the GNU General Public
 * License in all respects for all of the code used other than OpenSSL. If you
 * modify this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * The project's homepage is at http://sourceforge.net/projects/rsyncrypto
 */


#include "rsyncrypto.h"
#include "file.h"
#include "crypto.h"
#include "argtable2.h"


void usage()
{
    fprintf(stderr, "Usage: " PACKAGE_NAME " <src> <dst> <keys> <publickey file>\n"
            "Options:\n"
            "-h                   Help - this page\n"
            "-d                   Decrypt.\n"
            "-r                   <src> <dst> and <keys> are all directory names. The\n"
            "                     encryption will apply to all files in them recursively\n"
            "-c                   Only encrypt changed files - works only in recursive mode\n"
            "--trim               Number of directory entries to trim from the begining of\n"
            "                     the path. Default is 1\n"
            "--delete             In recursive mode, delete files in <dst> not in <src>\n"
            "--delete-keys        In recursive mode, delete also keys. Implies --detelte\n"
            "--filelist           <src> is a file (or \"-\" for stdin) with file and directory\n"
            "                     names to process.\n"
            "-b keysize           Must be one of 128, 192 or 256 bits. Encryption only.\n"
            "--fr                 Force new rollover parameters, even if previous encryption\n"
            "                     used a different setting.\n"
            "--fk                 Force new key size, even if previous encryption used a\n"
            "                     different setting\n"
            "--no-archive-mode    Do not try to preserve timestamps, permissions etc.\n"
            "--gzip               path to gzip program to use\n"
            "\nAdvance options:\n"
            "--roll-win           Rollover window size. Default is 8192 byte\n"
            "--roll-min           Minimal number of guaranteed non-rolled bytes. Default 8192\n"
            "--roll-sensitivity   How sensitive are we to cutting a block. Default is\n"
            "                     \"roll-win\"\n\n"
            "Currently only AES encryption is supported\n");

    exit(0);
}

startup_options options;

void parse_cmdline( int argc, char *argv[] )
{
    int nerrors=arg_parse( argc, argv, options.argtable );

    if( EXISTS(trim) && !EXISTS(recurse) && !EXISTS(filelist) )
        throw rscerror("Cannot trim names when not doing directory recursion");
    if( EXISTS(delkey) )
        ARG(del).count=1;

    // Some sanity check of the options
    if( EXISTS(decrypt) ) {
        if( EXISTS(keysize) )
            throw rscerror("Cannot specify key size for decryption");
        if( EXISTS(fr) || EXISTS(fk) )
            throw rscerror("\"force\" options incompatible with -d option");
        if( strcmp(FILENAME(src), "-")==0 ) {
            // Plaintext file is standard input/output
            if( !EXISTS(noarch) ) {
                throw rscerror("Must use \"--no-archive-mode\" if plaintext file is stdin");
            }
        }
    }

    // Apply default values
    if( !EXISTS(rollsens) )
        VAL(rollsens)=VAL(rollmin);
}

int main( int argc, char *argv[] )
{
    int ret=0;
    
    ERR_load_crypto_strings();

    try {
        parse_cmdline( argc, argv );

        if( EXISTS(help) )
            usage();

        RSA *rsa_key=extract_private_key(FILENAME(master));
        if( rsa_key==NULL ) {
            rsa_key=extract_public_key(FILENAME(master));
        }

        const char *opname=NULL;
        encryptfunc op;

        if( EXISTS(decrypt) ) {
            op=file_decrypt;
            opname="Decrypting";
        } else {
            op=file_encrypt;
            opname="Encrypting";
        }

        if( EXISTS(recurse) ) {
            dir_encrypt(FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key, op, opname);
        } else if( EXISTS(filelist) ) {
            filelist_encrypt( FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key, op, opname);
        } else {
            op(FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key);
        }
    } catch( const rscerror &err ) {
        std::cerr<<err.error()<<std::endl;
        ret=1;
    }

    return ret;
}
