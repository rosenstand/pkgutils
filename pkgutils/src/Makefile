include ../Makefile.h

CC = gcc -std=c99
CFLAGS += -Wall -I../include -DVERSION=\"$(VERSION)\"
CFLAGS += $(LIBARCHIVE_CFLAGS)
LDFLAGS += -L. -larchive
LDFLAGS += $(LIBARCHIVE_LDFLAGS)

OBJS = list.o misc.o libpkgdb.o libpkgadd.o libpkgrm.o filemode.o

%.o: %.c
	$(CC) -fPIC $(CFLAGS) -c $<

all: libpkgutils pkgadd pkgrm pkginfo pkgutils pkgmk rejmerge

libpkgutils.a:
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

libpkgutils.so.$(LIBVERSION):
	$(CC) -shared -Wl,-soname,libpkgutils.so $(LDFLAGS) \
		-o libpkgutils.so.$(LIBVERSION) $(OBJS)
	ln -s libpkgutils.so.$(LIBVERSION) libpkgutils.so

libpkgutils: $(OBJS) libpkgutils.so.$(LIBVERSION) libpkgutils.a

pkgadd: pkgadd.c
	$(CC) $(CFLAGS) $@.c $(LDFLAGS) -o $@ -lpkgutils

pkgrm: pkgrm.c
	$(CC) $(CFLAGS) $@.c $(LDFLAGS) -o $@ -lpkgutils

pkginfo: pkginfo.c
	$(CC) $(CFLAGS) $@.c $(LDFLAGS) -o $@ -lpkgutils

pkgutils: pkgadd.c pkgrm.c pkginfo.c pkgutils.c
	$(CC) $(CFLAGS) -DSTATIC -static pkgadd.c pkgrm.c pkginfo.c $@.c \
		 -o $@ libpkgutils.a $(LDFLAGS)

pkgmk: pkgmk.in
	sed -e "s/#VERSION#/$(VERSION)/" $< > $@

rejmerge: rejmerge.in
	sed -e "s/#VERSION#/$(VERSION)/" $< > $@

install: all
	install -m 0755 -D pkgadd $(BINDIR)/pkgadd
	install -m 0755 -D pkgrm $(BINDIR)/pkgrm
	install -m 0755 -D pkginfo $(BINDIR)/pkginfo
	install -m 0755 -D pkgutils $(BINDIR)/pkgutils
	install -m 0755 -D pkgmk $(BINDIR)/pkgutils
	install -m 0755 -D rejmerge $(BINDIR)/rejmerge
	install -m 0755 -D libpkgutils.so.$(LIBVERSION) \
	         $(LIBDIR)/libpkgutils.so.$(LIBVERSION)
	ln -s libpkgutils.so.$(LIBVERSION) $(LIBDIR)/libpkgutils.so
	install -m 0644 -D ../include/filemode.h $(INCDIR)/pkgutils/filemode.h
	install -m 0644 -D ../include/misc.h $(INCDIR)/pkgutils/misc.h
	install -m 0644 -D ../include/types.h $(INCDIR)/pkgutils/types.h
	install -m 0644 -D ../include/pkgutils.h $(INCDIR)/pkgutils/pkgutils.h
	install -m 0644 -D ../include/list.h $(INCDIR)/pkgutils/list.h
	install -m 0644 -D ../etc/pkgadd.conf $(ETCDIR)/pkgadd.conf
	install -m 0644 -D ../etc/pkgmk.conf $(ETCDIR)/pkgmk.conf
	install -m 0644 -D ../etc/rejmerge.conf $(ETCDIR)/rejmerge.conf

clean:
	-rm -f *.o                            \
		libpkgutils.so* libpkgutils.a \
		pkgadd pkgrm pkginfo pkgutils \
		pkgmk rejmerge
