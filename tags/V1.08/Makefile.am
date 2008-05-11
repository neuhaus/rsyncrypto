bin_PROGRAMS = rsyncrypto
noinst_PROGRAMS = blocksizes
man1_MANS = rsyncrypto.man

rsyncrypto_SOURCES = main.cpp crypto.cpp crypt_key.cpp aes_crypt.cpp file.cpp process.cpp \
	redir.cpp filemap.cpp bufferfd.cpp
blocksizes_SOURCES = crypt_key.cpp aes_crypt.cpp blocksizes.cpp

.PHONY: test docs
test: rsyncrypto
	[ -d ./tests ] || mkdir ./tests
	TRGT="$$(pwd)" ; SRC="$$(cd "$(top_srcdir)/tests" ; pwd)" ; export SRC ; cd $${TRGT}/tests && \
	"$$SRC/regtest" "$$TRGT/rsyncrypto" v

docs: rsyncrypto.html

rsyncrypto.html: rsyncrypto.man
	man2html "$(top_srcdir)/rsyncrypto.man" | tail -n +3 > rsyncrypto.html.tmp
	mv rsyncrypto.html.tmp rsyncrypto.html
