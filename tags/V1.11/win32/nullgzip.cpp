#include <windows.h>
#include <stdio.h>

int main(int argc, char *argv[] )
{
    unsigned char buffer[8192];
    HANDLE hInput, hOutput;

    hInput=GetStdHandle(STD_INPUT_HANDLE);
    hOutput=GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD numread;
    BOOL success;

    while( (success=ReadFile(hInput, buffer, sizeof(buffer), &numread, NULL )) && numread>0 ) {
        DWORD written;
        if( !WriteFile(hOutput, buffer, numread, &written, NULL ) ) {
            fprintf(stderr, "Error in writing: %d\n", GetLastError() );
            return 1;
        }
    }

    DWORD error=GetLastError();

    if( !success && error!=ERROR_BROKEN_PIPE ) {
        fprintf(stderr, "Error in reading: %d\n", GetLastError() );
        return 1;
    }

    return 0;
}