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
#include "process.h"

process_ctl::process_ctl( const char *cmd, const autofd &input, const autofd &output ...)
{
    pid=fork();
    if( pid==-1 )
        throw rscerror("Error creating child process", errno, cmd);
    
    if( pid==0 ) {
        va_list args;
        // The following line generates two false warnings on gcc 3.3.5. gcc 3.4 does not complain.
        va_start(args, output);
        int numargs=0;
        while( va_arg(args, char *)!=NULL )
            ++numargs;
        va_end(args);
        // The following line generates two false warnings on gcc 3.3.5. gcc 3.4 does not complain.
        va_start(args, output);

        auto_array<char *> arguments(new char *[numargs+1]);

        for( int i=0; (arguments[i]=va_arg(args, char *))!=NULL; ++i )
            ;
        va_end(args);

        if( input.valid() ) {
            dup2( input.get(), STDIN_FILENO );
        }
        if( output.valid() ) {
            dup2( output.get(), STDOUT_FILENO );
        }
        execvp(cmd, arguments.get() );
    }
}

int process_ctl::wait() const
{
    int childstatus;

    do {
        waitpid(pid, &childstatus, 0);
    } while( !WIFEXITED(childstatus) );

    return WEXITSTATUS(childstatus);
}