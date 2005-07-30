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
 * The project's homepage is at http://sourceforge.net/projects/rsyncrypto
 */
#include "rsyncrypto.h"
#include "crypt_key.h"

startup_options options;

int main( int argc, char *argv[] )
{
    if( argc<4 ) {
        std::cout<<"Usage: blocksizes winsize minsize modulo <times>"<<std::endl;
        exit(0);
    }

    size_t times=0;

    options.rollwin=strtoul(argv[1], NULL, 10);
    options.rollmin=strtoul(argv[2], NULL, 10);
    options.rollsens=strtoul(argv[3], NULL, 10);

    if( argc>4 )
        times=strtoul(argv[4], NULL, 10);

    options.verbosity=3;

    autofd random(open("/dev/urandom", O_RDONLY));
    std::auto_ptr<key> testkey(key::new_key( key::CYPHER_AES, 0, options.rollwin, options.rollmin,
                options.rollsens ));

    bool border=true;
    do {
        if( border ) {
            testkey->init_encrypt();
            --times;
        }

        unsigned char buff;
        random.read( &buff, 1 );

        border=testkey->calc_boundry( buff );
    } while( times>0 );
}