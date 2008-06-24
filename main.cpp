/*
 * rsyncrypto - an rsync friendly encryption
 * Copyright (C) 2005-2008 Shachar Shemesh for Lingnu Open Source Consulting ltd.
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
 * The project's homepage is at http://rsyncrypto.lingnu.com/
 */

#include "precomp.h"
#include "rsyncrypto.h"
#include "file.h"
#include "crypto.h"
#include "argtable2.h"
#include "filemap.h"

std::auto_ptr<std::ostream> changes_log;

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
    int nerrors=arg_parse( argc, argv, options.argtable );
	if( nerrors!=0 ) {
		std::cerr<<"Incorrect arguments"<<std::endl;
		usage();
    }

    if( EXISTS(trim) && !EXISTS(recurse) && !EXISTS(filelist) )
        throw rscerror("Cannot trim names when not doing directory recursion or filelist");
    if( EXISTS(nameenc) && !EXISTS(recurse) && !EXISTS(filelist) )
        throw rscerror("Cannot encrypt names when not doing directory recursion or filelist");
    if( EXISTS(delkey) )
        ARG(del).count=1;

    // Some sanity check of the options
    if( EXISTS(decrypt) ) {
        if( EXISTS(keysize) )
            throw rscerror("Cannot specify key size for decryption");
        if( EXISTS(fr) || EXISTS(fk) )
            throw rscerror("\"force\" options incompatible with -d option");
        if( strcmp(FILENAME(src), "-")==0 && !EXISTS(filelist) ) {
            // Plaintext file is standard input/output
            if( !EXISTS(noarch) ) {
                throw rscerror("Must use \"--no-archive-mode\" if plaintext file is stdin");
            }
        }
        if( EXISTS(export_changes) ) {
            throw rscerror("Log export not supported in decrypt mode");
        }
    }

    // Apply default values
    if( !EXISTS(rollsens) )
        VAL(rollsens)=VAL(rollmin);
    if( !EXISTS(noatime) )
        VAL(noatime)=1;
    if( !EXISTS(mod_win) )
        VAL(mod_win)=0;
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

#if HAVE_NOATIME
        // Sometimes we can always use O_NOATIME without a problem
        if( VAL(noatime)==1 && geteuid()==0 ) {
            // We are root - O_NOATIME will succeed anyways
            VAL(noatime)=2;
        }
#endif

        RSA *rsa_key=extract_private_key(FILENAME(master));
        if( rsa_key==NULL ) {
            rsa_key=extract_public_key(FILENAME(master));

            if( rsa_key==NULL ) {
                throw rscerror( "Couldn't parse RSA key", 0, FILENAME(master) );
            }
        }

        if( EXISTS(export_changes) ) {
            changes_log=std::auto_ptr<std::ofstream>(new std::ofstream(FILENAME(export_changes), std::ofstream::trunc));
        }

        const char *opname=NULL;
	encryptfunc op;
	namefunc srcnameop=name_concat, dstnameop=name_concat, keynameop=name_concat;
	bool encrypt=true;

	if( EXISTS(decrypt) ) {
	    op=file_decrypt;
	    opname="Decrypting";
	    encrypt=false;
	} else {
	    op=file_encrypt;
	    opname="Encrypting";
	}

	if( EXISTS(nameenc) ) {
            if( encrypt ) {
		dstnameop=filemap::namecat_encrypt;
		keynameop=dstnameop;
	    } else {
		if( EXISTS(recurse) || EXISTS(filelist) ) {
		    // First decrypt the encrypted file list
		    file_decrypt(autofd::combine_paths(FILENAME(src), FILEMAPNAME).c_str(),
                        FILENAME(nameenc), autofd::combine_paths(FILENAME(key),
                        FILEMAPNAME).c_str(), rsa_key, NULL );
		}

		dstnameop=filemap::namecat_decrypt;
	    }
	    filemap::fill_map(FILENAME(nameenc), encrypt);
	}

	if( EXISTS(recurse) || EXISTS(filelist) ) {
	    if( EXISTS(recurse) )
		dir_encrypt(FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key, op, opname,
			dstnameop, keynameop);
	    else
		filelist_encrypt( FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key, op, opname,
			srcnameop, dstnameop, keynameop);

	    if( encrypt && EXISTS(nameenc) ) {
		// Write the (possibly changed) filelist back to the file
		filemap::write_map(FILENAME(nameenc));
		// Encrypt the filelist file itself
		file_encrypt(FILENAME(nameenc), autofd::combine_paths(FILENAME(dst), FILEMAPNAME).
                    c_str(), autofd::combine_paths(FILENAME(key), FILEMAPNAME).c_str(), rsa_key,
                    NULL );
	    }
	} else {
            struct stat status, *pstat=&status;
            if( strcmp( FILENAME(src), "-" )!=0 ) {
                status=autofd::stat(FILENAME(src));
            } else
                pstat=NULL;

	    op( FILENAME(src), FILENAME(dst), FILENAME(key), rsa_key, pstat );
	}
    } catch( const rscerror &err ) {
        std::cerr<<err.error()<<std::endl;
        ret=1;
    }
    
    return ret;
}
