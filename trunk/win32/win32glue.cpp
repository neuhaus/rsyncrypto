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
 * The project's homepage is at http://rsyncrypto.lingnu.com
 */

#include "../precomp.h"

int Error2errno( DWORD Error ) {
	switch( Error ) {
	case ERROR_SUCCESS:
            return 0;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case ERROR_INVALID_DRIVE:
        return ENODEV;
    case ERROR_ACCESS_DENIED:
        return EACCES;
    case ERROR_TOO_MANY_OPEN_FILES:
        return EMFILE;
    case ERROR_INVALID_HANDLE:
        return EBADF;
    case ERROR_ARENA_TRASHED:
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_INVALID_BLOCK:
    case ERROR_OUTOFMEMORY:
        return ENOMEM;
    case ERROR_BAD_ENVIRONMENT:
        return E2BIG;
    case ERROR_BAD_FORMAT:
        return ENOEXEC;
    case ERROR_CURRENT_DIRECTORY:
        return EBUSY;
    case ERROR_NOT_SAME_DEVICE:
        return EXDEV;
    case ERROR_WRITE_PROTECT:
        return EROFS;
    case ERROR_BAD_UNIT:
        return ENODEV;
    case ERROR_NOT_READY:
        return ENXIO; // An approximation
        // return ENOMEDIUM;
    case ERROR_BAD_LENGTH:
        return E2BIG;
    case ERROR_WRITE_FAULT:
    case ERROR_READ_FAULT:
        return EIO;
    case ERROR_SHARING_VIOLATION:
        return EBUSY;
    default:
        return Error;
    }
}
