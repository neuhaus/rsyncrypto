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

int parse_cmdline( int argc, char *argv[] )
{
    enum option_type { ROLL_WIN=1, ROLL_MIN, ROLL_SENS, FR, FK, GZIP, NO_ARCHIVE, TRIM, DELETE, FILELIST };
    int c;
    const struct option long_options[]={
	{ "roll-win", 1, NULL, ROLL_WIN },
	{ "roll-min", 1, NULL, ROLL_MIN },
	{ "roll-sensitivity", 1, NULL, ROLL_SENS },
	{ "fr", 0, NULL, FR },
	{ "fk", 0, NULL, FK },
	{ "gzip", 1, NULL, GZIP },
        { "help", 0, NULL, 'h' },
        { "verbose", 0, NULL, 'v' },
        { "no-archive-mode", 0, NULL, NO_ARCHIVE },
        { "trim", 1, NULL, TRIM },
        { "delete", 0, NULL, DELETE },
        { "filelist", 0, NULL, FILELIST },
	{ NULL, 0, NULL, 0 }};
    
    while( (c=getopt_long(argc, argv, "b:cdhrv", long_options, NULL ))!=-1 )
    {
        switch(c) {
        case 'h':
            usage();
            break;
        case 'b':
            if( options.keysize!=0 ) {
                // Can't say "-b" twice
                throw rscerror("-b option specified twice");
            }
            options.keysize=atol(optarg);
            if( options.keysize==0 ) {
                // Invalid option
                throw rscerror("Invalid -b parameter given");
            }
            break;
        case 'c':
            if( options.changed ) {
                throw rscerror("-c option specified twice");
            }
            options.changed=true;
            break;
        case 'd':
            if( options.decrypt ) {
                throw rscerror("-d option given twice");
            }
            options.decrypt=true;
            break;
        case 'v':
            options.verbosity++;
            break;
        case 'r':
            if( options.recurse ) {
                throw rscerror("-r option given twice");
            }
            options.recurse=true;
            break;
        case ROLL_WIN:
            if( options.rollwin!=0 )
                throw rscerror("--roll-win option given twice");
            options.rollwin=strtoul(optarg, NULL, 10);
            if( options.rollwin==0 )
                throw rscerror("Invalid --roll-win parameter given");
            break;
        case ROLL_MIN:
            if( options.rollmin!=0 )
                throw rscerror("--roll-min option given twice");
            options.rollmin=strtoul(optarg, NULL, 10);
            if( options.rollmin==0 )
                throw rscerror("Invalid --roll-min parameter given");
            break;
        case ROLL_SENS:
            if( options.rollsens!=0 )
                throw rscerror("--roll-sensitivity option given twice");
            options.rollsens=strtoul(optarg, NULL, 10);
            if( options.rollsens==0 )
                throw rscerror("Invalid --roll-sensitivity parameter given");
            break;
        case FR:
            if( options.fr )
                throw rscerror("--fr option given twice");
            options.fr=true;
            break;
        case FK:
            if( options.fk )
                throw rscerror("--fr option given twice");
            options.fr=true;
            break;
        case GZIP:
            if( options.gzip!=NULL )
                throw rscerror("--gzip option given twice");
            options.gzip=optarg;
            break;
        case NO_ARCHIVE:
            if( !options.archive )
                throw rscerror("--no-archive option given twice");
            options.archive=false;
            break;
        case TRIM:
            if( options.trim!=-1 )
                throw rscerror("--trim option given twice");
            if( !options.recurse && !options.filelist )
                throw rscerror("Cannot trim names when not doing directory recursion");
            options.trim=atoi(optarg);
            break;
        case DELETE:
            if( options.del )
                throw rscerror("--delete option given twice");
            options.del=true;
            break;
        case FILELIST:
            options.filelist=true;
            break;
        case '?':
            throw rscerror("Unrecognized option given");
            break;
        default:
            throw rscerror("Internal parameter processing error");
            break;
        }
    }

    // Some sanity check of the options
    if( options.decrypt ) {
        if( options.keysize!=0 )
            throw rscerror("Cannot specify key size for decryption");
        if( options.fr || options.fk )
            throw rscerror("\"force\" options incompatible with -d option");
        if( strcmp(argv[optind], "-")==0 ) {
            // Plaintext file is standard input/output
            if( options.archive ) {
                throw rscerror("Must use \"--no-archive-mode\" if plaintext file is stdin");
            }
        }
    }

    // Apply default values
    if( options.rollwin==0 )
        options.rollwin=8192;
    if( options.rollmin==0 )
        options.rollmin=8192;
    if( options.rollsens==0 )
        options.rollsens=options.rollmin;
    if( options.trim==-1 )
        options.trim=1;
    if( options.gzip==NULL )
        options.gzip="gzip";

    return optind;
}

int main( int argc, char *argv[] )
{
    int ret=0;
    
    ERR_load_crypto_strings();

    try {
        int argskip=parse_cmdline( argc, argv );

        argv+=argskip;
        argc-=argskip;

        if( argc!=4 )
            usage();

        RSA *rsa_key=extract_private_key(argv[3]);
        if( rsa_key==NULL ) {
            rsa_key=extract_public_key(argv[3]);
        }

        const char *opname=NULL;
        encryptfunc op;

        if( options.decrypt ) {
            op=file_decrypt;
            opname="Decrypting";
        } else {
            op=file_encrypt;
            opname="Encrypting";
        }

        if( options.recurse ) {
            dir_encrypt(argv[0], argv[1], argv[2], rsa_key, op, opname);
        } else if( options.filelist ) {
            filelist_encrypt( argv[0], argv[1], argv[2], rsa_key, op, opname);
        } else {
            op(argv[0], argv[1], argv[2], rsa_key);
        }
    } catch( const rscerror &err ) {
        std::cerr<<err.error()<<std::endl;
        ret=1;
    }

    return ret;
}
