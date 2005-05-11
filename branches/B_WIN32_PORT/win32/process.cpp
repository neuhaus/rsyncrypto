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
#include "../rsyncrypto.h"
#include "process.h"
#include "win32redir.h"

process_ctl::process_ctl( char *cmd, redir *input, redir *output, redir *error, ... )
{
    STARTUPINFO siStartInfo; 
    // Set up members of the STARTUPINFO structure. 
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.dwFlags= STARTF_FORCEOFFFEEDBACK|STARTF_USESTDHANDLES;
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    HANDLE hProcess=GetCurrentProcess();

    autohandle input_stream;

    win32_redir_opaq inop, outop, errop;
    inop.si=outop.si=errop.si=&siStartInfo;

    if( input!=NULL )
        input->parent_redirect( STDIN_FILENO, &inop );
    if( output!=NULL )
        output->parent_redirect( STDOUT_FILENO, &outop );
    if( error!=NULL )
        error->parent_redirect( STDERR_FILENO, &errop );

    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory( &pInfo, sizeof(pInfo) );    
    
    std::string cmdline;
    cmdline="\"";
    cmdline+=cmd;
    cmdline+="\" ";

    va_list args;
    va_start(args, output);
    const char *param;
    while( (param=va_arg(args, const char *))!=NULL ) {
        if( *param=='\0' || strchr( param, ' ' )==NULL ) {
            cmdline+="\"";
            cmdline+=param;
            cmdline+="\" ";
        } else {
            cmdline+=param;
            cmdline+=" ";
        }
    }

    va_end(args);

    ODS("CreateProcess\n");
    // Create the child process.
    if( CreateProcess(NULL, 
        const_cast<char *>(cmdline.c_str()),       // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        CREATE_DEFAULT_ERROR_MODE|NORMAL_PRIORITY_CLASS, // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &pInfo) )       // receives PROCESS_INFORMATION 
    {
        hProcess=autohandle(pInfo.hProcess);
        hThread=autohandle(pInfo.hThread);
    } else
    {
        // CreateProcess failed
        throw rscerror("Child process not created", GetLastError() );
    }
}

int process_ctl::wait() const
{
    ODS("Wait for %08x to exit\n", hProcess);
    WaitForSingleObject(hProcess, INFINITE);
    DWORD exitcode=-1;
    GetExitCodeProcess( hProcess, &exitcode );
    ODS("Process %08x quit with error code %d\n", hProcess, exitcode );

    return exitcode;
}
