#ifndef BUFFERFD_H
#define BUFFERFD_H

class read_bufferfd : public autofd {
    static const size_t DEFAULT_BUF_SIZE;
    const size_t buf_size;
    auto_array<char> buffer;
    mutable int startpos, endpos;

    ssize_t buffer_copy( void *buf, size_t offset, size_t count ) const;
public:
    explicit read_bufferfd( size_t bufsize=DEFAULT_BUF_SIZE ) : buf_size(bufsize),
	buffer(new char [bufsize]), startpos(0), endpos(0)
    {
    }

    explicit read_bufferfd( autofd &rhs, size_t bufsize=DEFAULT_BUF_SIZE ) : autofd(rhs),
	buf_size(bufsize), buffer(new char [bufsize]), startpos(0), endpos(0)
    {
    }
    // We're actually ok with both default copy constructor AND operator= !!!

    ssize_t read( void *buf, size_t count ) const;
};

const size_t read_bufferfd::DEFAULT_BUF_SIZE=8192;

#endif // BUFFERFD_H
