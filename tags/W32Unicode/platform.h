// Generic platform dependent support
#ifndef PLATFORM_H
#define PLATFORM_H

// Convert from ANSI to TCHAR and vice versa. len=0 means treat the string as NULL terminated
// If utf8 is true, the ANSI string is assumed to be UTF8 encoded. Otherwise, it is locale encoded.
// On none Unicode builds, the ANSI string is always assumed to be locale encoded.
TSTRING a2t( const char *str, size_t len=0, bool utf8=false );
std::string t2a( const TCHAR *str, size_t len=0, bool utf8=false );

#endif // PLATFORM_H