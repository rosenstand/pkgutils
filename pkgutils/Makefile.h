VERSION=1.1
LIBVERSION=1.0
LIBARCHIVE_LDFLAGS=-larchive -lz
PREFIX?=/usr/local
BINDIR=$(DESTDIR)$(PREFIX)/bin
MANDIR=$(DESTDIR)$(PREFIX)/man
LIBDIR=$(DESTDIR)$(PREFIX)/lib
INCDIR=$(DESTDIR)$(PREFIX)/include
ETCDIR=$(DESTDIR)/etc
RANLIB=ranlib
