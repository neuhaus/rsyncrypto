#ifndef PROCESS_H
#if defined(_WIN32)
#include "win32/process.h"
#else
#define PROCESS_H
    /* pipe, fork and run gzip */
    autofd ipipe;
    {
        int iopipe[2];
        if( pipe(iopipe)!=0 )
            throw rscerror("Couldn't create pipe", errno);

        switch(child_pid=fork())
        {
        case 0:
            /* child */
            /* Redirect stdout to the pipe, and gzip the fromfd */
            close(iopipe[0]);
            dup2(iopipe[1],STDOUT_FILENO);
            close(iopipe[1]);
            dup2(fromfd, STDIN_FILENO);
            close(fromfd);
            close(tofd);
            execlp( FILENAME(gzip), FILENAME(gzip), "--rsyncable", (char *)NULL);
            exit(1);
            break;
        case -1:
            /* Running gzip failed */
            throw rscerror("Failed to spawn child process", errno);
            break;
        default:
            /* Parent */
            close(iopipe[1]);
            ipipe=autofd(iopipe[0]);
            break;
        }
    }
#endif // _WIN32
#endif // PROCESS_H