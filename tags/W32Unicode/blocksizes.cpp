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
 * The project's homepage is at http://rsyncrypto.lingnu.com/
 */
#include <precomp.h>
#include "rsyncrypto.h"
#include "crypt_key.h"
#include "random.h"

startup_options options;

int main( int argc, char *argv[] )
{
    if( argc<4 ) {
        std::cout<<"Usage: blocksizes winsize minsize modulo <times>"<<std::endl;
        exit(0);
    }

    size_t times=0;

    VAL(rollwin)=strtoul(argv[1], NULL, 10);
    VAL(rollmin)=strtoul(argv[2], NULL, 10);
    VAL(rollsens)=strtoul(argv[3], NULL, 10);

    if( argc>4 )
        times=strtoul(argv[4], NULL, 10);

    ARG(verbosity).count=3;

    std::auto_ptr<key> testkey(key::new_key( key::CYPHER_AES, 0, VAL(rollwin), VAL(rollmin),
                VAL(rollsens) ));

    bool border=true;
    do {
        if( border ) {
            testkey->init_encrypt();
            --times;
        }

        unsigned char buff;
        random::rand( &buff, 1 );

        border=testkey->calc_boundry( buff );
    } while( times>0 );

    return 0;
}
