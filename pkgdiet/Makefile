VERSION=1.0
PREFIX=/usr

CC=gcc
CFLAGS+=-Wall -DPREFIX=\"$(PREFIX)\" -DVERSION=\"$(VERSION)\"
LDFLAGS+=-larchive

pkgdiet: pkgdiet.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

all: pkgdiet

clean:
	-rm -f pkgdiet

install: all
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/etc/
	install -m 0755 pkgdiet $(DESTDIR)$(PREFIX)/bin/
	install -m 0644 pkgdiet.conf $(DESTDIR)$(PREFIX)/etc/pkgdiet.conf

SNAPSHOT=pkgdiet-`date +%Y%m%d`
snapshot:
	rm -rf /tmp/$(SNAPSHOT)
	svn export . /tmp/$(SNAPSHOT)
	(cd /tmp && tar -c $(SNAPSHOT) | bzip2 > $(SNAPSHOT).tar.bz2)

release:
	make snapshot SNAPSHOT=pkgdiet-$(VERSION)
