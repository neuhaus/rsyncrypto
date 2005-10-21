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
#include "../redir.h"
#include "win32redir.h"

void redir_pipe::child_redirect( int redir_type, void *plat_opaq )
{
}
void redir_pipe::parent_redirect( int redir_type, void *plat_opaq )
{
    win32_redir_opaq *opaq=static_cast<win32_redir_opaq *>(plat_opaq);

    if( redir_type==STDIN_FILENO ) {
        opaq->handle=get_read().Duplicate(true);
        opaq->si->hStdInput=opaq->handle;
        ODS("Pipe child input=%08x\n", opaq->si->hStdInput);
        clear_read();
    } else {
        opaq->handle=get_write().Duplicate(true);
        clear_write();
        if( redir_type==STDOUT_FILENO ) {
            opaq->si->hStdOutput=opaq->handle;
            ODS("Pipe child output=%08x\n", opaq->si->hStdOutput);
        } else {
            opaq->si->hStdError=opaq->handle;
            ODS("Pipe child error=%08x\n", opaq->si->hStdError);
        }
    }
}

void redir_fd::child_redirect( int redir_type, void *plat_opaq )
{
}
void redir_fd::parent_redirect( int redir_type, void *plat_opaq )
{
    win32_redir_opaq *opaq=static_cast<win32_redir_opaq *>(plat_opaq);

    opaq->handle=Duplicate(true);

    switch(redir_type)
    {
    case STDIN_FILENO:
        opaq->si->hStdInput=opaq->handle;
        ODS("File child input=%08x\n", opaq->si->hStdInput);
        break;
    case STDOUT_FILENO:
        opaq->si->hStdOutput=opaq->handle;
        ODS("File child output=%08x\n", opaq->si->hStdOutput);
        break;
    case STDERR_FILENO:
        opaq->si->hStdError=opaq->handle;
        ODS("File child error=%08x\n", opaq->si->hStdError);
        break;
    }

    clear();
}

void redir_null::child_redirect( int redir_type, void *plat_opaq )
{
}
void redir_null::parent_redirect( int redir_type, void *plat_opaq )
{
}