#ifndef TCHAR_FILLER_H
#define TCHAR_FILLER_H

/* This file is used to stub-define the functions provided by Windows' tchar support.
 * These functions are just other names for the standard string manipulation functions
 */

#define TCHAR char

#define _tcscmp strcmp
#define _tcscpy strcpy

#endif // TCHAR_FILLER_H
