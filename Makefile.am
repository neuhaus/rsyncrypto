bin_PROGRAMS = rsyncrypto rsyncrypto_recover
noinst_PROGRAMS = blocksizes
man1_MANS = rsyncrypto.man rsyncrypto_recover.man

rsyncrypto_SOURCES = main.cpp crypto.cpp crypt_key.cpp aes_crypt.cpp file.cpp process.cpp \
	redir.cpp filemap.cpp bufferfd.cpp
blocksizes_SOURCES = crypt_key.cpp aes_crypt.cpp blocksizes.cpp

rsyncrypto_recover_SOURCES=recover.cpp

.PHONY: test docs
test: rsyncrypto
	[ -d ./tests ] || mkdir ./tests
	TRGT="$$(pwd)" ; SRC="$$(cd "$(top_srcdir)/tests" ; pwd)" ; export SRC ; cd $${TRGT}/tests && \
	"$$SRC/regtest" "$$TRGT/rsyncrypto" v

docs: rsyncrypto.html rsyncrypto_recover.html

%.html: %.man
	man2html "$<" | tail -n +3 > "$@.tmp"
	mv "$@.tmp" "$@"
