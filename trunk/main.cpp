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
#include "filelist.h"

void version()
{
    printf("%s by Shachar Shemesh\n", PACKAGE_STRING);
    printf("This program was developed as part of Lingnu Open Source Consulting's online\n"
	    "backup service. For further details check out http://www.lingnu.com\n");
}

void usage()
{
    fprintf(stderr, "%s ", PACKAGE_STRING );
    arg_print_syntax(stderr, options.argtable, "\n\n");
    arg_print_glossary(stderr, options.argtable, "%-22s %s\n");
    exit(0);
}

startup_options options;

void parse_cmdline( int argc, char *argv[] )
{
    // XXX int nerrors=
    arg_parse( argc, argv, options.argtable );

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

	if( EXISTS(version) ) {
	    version();
	    exit(0);
	}
	
        if( EXISTS(help) )
            usage();

        RSA *rsa_key=extract_private_key(FILENAME(master));
        if( rsa_key==NULL ) {
            rsa_key=extract_public_key(FILENAME(master));
        }

        const char *opname=NULL;
        encryptfunc op;
	namefunc nameop=name_concat, keynameop=name_concat;
	bool encrypt=true;

        if( EXISTS(decrypt) ) {
            op=file_decrypt;
            opname="Decrypting";
	    encrypt=false;
        } else {
            op=file_encrypt;
            opname="Encrypting";
        }

	if( EXISTS(metaenc) ) {
	    if( encrypt ) {
		nameop=metadata::namecat_encrypt;
		keynameop=nameop;
	    } else {
		if( EXISTS(recurse) || EXISTS(filelist) ) {
		    // First decrypt the encrypted file list
		    file_decrypt(autofd::combine_paths(FILENAME(src), FILELISTNAME).c_str(), FILENAME(metaenc),
			    autofd::combine_paths(FILENAME(key), FILELISTNAME).c_str(), rsa_key );
		}

		nameop=metadata::namecat_decrypt;
	    }
	    metadata::fill_map(FILENAME(metaenc), encrypt);
	}

        if( EXISTS(recurse) ) {
            dir_encrypt(FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key, op, opname, nameop, keynameop);
	    if( encrypt && EXISTS(metaenc) ) {
		// Write the (possibly changed) filelist back to the file
		metadata::write_map(FILENAME(metaenc));
		// Encrypt the filelist file itself
		file_encrypt(FILENAME(metaenc), autofd::combine_paths(FILENAME(dst), FILELISTNAME).c_str(),
			autofd::combine_paths(FILENAME(key), FILELISTNAME).c_str(), rsa_key );
	    }
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
