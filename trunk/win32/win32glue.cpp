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

// Unconditionally convert from ANSI to Wide
static auto_array<wchar_t> a2u( const char *str, size_t len, bool utf8 )
{
    int mbcs_len= len==0 ? -1 : len;
    int newlen=MultiByteToWideChar(utf8 ? CP_UTF8 : CP_ACP, MB_ERR_INVALID_CHARS, str, mbcs_len, NULL, 0 );

    if( newlen==0 ) {
        throw( rscerror( _T("Ansi to Unicode conversion error"), GetLastError()) );
    }

    auto_array<wchar_t> buffer(new wchar_t[newlen+1]);

    if( MultiByteToWideChar(utf8 ? CP_UTF8 : CP_ACP, MB_ERR_INVALID_CHARS, str, mbcs_len, buffer.get(), newlen )==0 ) {
        throw( rscerror( _T("Ansi to Unicode conversion error"), GetLastError()) );
    }

    buffer[newlen]=L'\0';

    return buffer;
}

// Unconditionally convert from Wide to ANSI
static auto_array<char> u2a( const wchar_t *str, size_t len, bool utf8 )
{
    int wide_len= len==0 ? -1 : len;
    DWORD flags= utf8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS;
    int newlen=WideCharToMultiByte(utf8 ? CP_UTF8 : CP_ACP, flags, str, wide_len, NULL, 0, NULL, NULL );

    if( newlen==0 ) {
        throw( rscerror( _T("Unicode to Ansi conversion error"), GetLastError()) );
    }

    auto_array<char> buffer(new char[newlen+1]);

    if( WideCharToMultiByte(utf8 ? CP_UTF8 : CP_ACP, flags, str, wide_len, buffer.get(), newlen, NULL, NULL )==0 ) {
        throw( rscerror( _T("Ansi to Unicode conversion error"), GetLastError()) );
    }

    buffer[newlen]='\0';

    return buffer;
}

#ifdef UNICODE

TSTRING a2t( const char *str, size_t len, bool utf8 )
{
    return a2u( str, len, utf8 ).get();
}

std::string t2a( const wchar_t *str, size_t len, bool utf8 )
{
    return u2a( str, len, utf8 ).get();
}

#else // ANSI build

TSTRING a2t( const char *str, size_t len, bool utf8 )
{
    if( !utf8 ) {
        // This is a NOP function
        if( len==0 )
            return std::string(str);
        else
            return std::string(str, len);
    }

    // We need to convert the string from UTF8 to the local locale
    return u2a( a2u( str, len, true ).get(), 0, false ).get();
}

std::string t2a( const TCHAR *str, size_t len, bool utf8 )
{
    // Does almost exactly the same as a2t in ANSI mode

    if( !utf8 ) {
        // This is a NOP function
        if( len==0 )
            return std::string(str);
        else
            return std::string(str, len);
    }

    // We need to convert the string from UTF8 to the local locale
    return u2a( a2u( str, len, false ).get(), 0, true ).get();
}

#endif