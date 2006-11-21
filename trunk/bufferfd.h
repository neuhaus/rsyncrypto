#ifndef BUFFERFD_H
#define BUFFERFD_H

class read_bufferfd : public autofd {
    static const size_t DEFAULT_BUF_SIZE;
    const size_t buf_size;
    auto_array<char> buffer;
    mutable int startpos, endpos;

    ssize_t buffer_copy( void *buf, size_t count ) const;
public:
    explicit read_bufferfd( size_t bufsize=DEFAULT_BUF_SIZE ) : buf_size(bufsize),
	buffer(new char [bufsize]), startpos(0), endpos(0)
    {
    }

    read_bufferfd( const autofd &rhs, size_t bufsize=DEFAULT_BUF_SIZE ) : autofd(rhs),
	buf_size(bufsize), buffer(new char [bufsize]), startpos(0), endpos(0)
    {
    }
    // We're actually ok with default copy constructor

    // We would be ok with default operator=, except that it needs to change some read-only vars
    read_bufferfd &operator=( const read_bufferfd &rhs ) {
	*static_cast<autofd *>(this)=rhs;
	const_cast<size_t &>(buf_size)=rhs.buf_size;
	buffer=const_cast<auto_array<char> &>(rhs.buffer);
	startpos=rhs.startpos;
	endpos=rhs.endpos;

	return *this;
    }

    ssize_t read( void *buf, size_t count ) const;
};

class write_bufferfd : public autofd {
    static const size_t DEFAULT_BUF_SIZE;
    const size_t buf_size;
    auto_array<char> buffer;
    mutable int buffill;
public:
    write_bufferfd( size_t bufsize=DEFAULT_BUF_SIZE ) : buf_size(bufsize),
	buffer(new char [bufsize]), buffill(0)
    {
    }
    write_bufferfd( const autofd &rhs, size_t bufsize=DEFAULT_BUF_SIZE ) : autofd(rhs),
	buf_size(bufsize), buffer(new char [bufsize]), buffill(0)
    {
    }
    // We're ok with the default copy constructor

    ~write_bufferfd()
    {
	flush();
    }

    ssize_t write( void *buf, size_t count );
    void flush();
    void clear() {
	flush();
	autofd::clear();
    }
};

#endif // BUFFERFD_H
