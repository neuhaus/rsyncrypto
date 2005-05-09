#ifndef REDIR_H
#define REDIR_H

#include "autopipe.h"

// This is a base class for process output redirection
class redir {
public:
    virtual void child_redirect( int redir_type )=0;
    virtual void parent_redirect( int redir_type )=0;
};

class redir_pipe : public redir, public autopipe {
public:
    redir_pipe( const autopipe &that ) : autopipe(that)
    {
    }
    explicit redir_pipe( size_t pipe_size=4096) : autopipe(pipe_size)
    {
    }
    virtual ~redir_pipe()
    {
    }
    virtual void child_redirect( int redir_type );
    virtual void parent_redirect( int redir_type );
};

class redir_fd : public redir, public autofd {
public:
    redir_fd(const autofd &that) : autofd(that)
    {
    }
    redir_fd() : autofd()
    {
    }
    explicit redir_fd( file_t fd ) : autofd( fd )
    {
    }
#if defined(EXCEPT_CLASS)
    redir_fd( file_t fd, bool except ) : autofd( fd, except )
    {
    }
    redir_fd( const char *pathname, int flags, mode_t mode ) : autofd( pathname, flags, mode )
    {
    }
#endif
    virtual ~redir_fd()
    {
    }
    virtual void child_redirect( int redir_type );
    virtual void parent_redirect( int redir_type );
};

// Do nothing redirection
class redir_null : public redir {
public:
    virtual void child_redirect( int redir_type );
    virtual void parent_redirect( int redir_type );
};

#endif // REDIR_H
