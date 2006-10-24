include ../Makefile.h

OBJS = pkgadd.8 pkgmk.8 pkginfo.8 pkgrm.8 rejmerge.8

%: %.in
	sed -e "s/#VERSION#/$(VERSION)/" $< > $@

all: $(OBJS)

install: all
	install -m 0644 -D pkgadd.8   $(MANDIR)/man8/pkgadd.8
	install -m 0644 -D pkgrm.8    $(MANDIR)/man8/pkgrm.8
	install -m 0644 -D pkginfo.8  $(MANDIR)/man8/pkginfo.8
	install -m 0644 -D pkgmk.8    $(MANDIR)/man8/pkgmk.8
	install -m 0644 -D rejmerge.8 $(MANDIR)/man8/rejmerge.8

clean:
	-rm -f $(OBJS)
