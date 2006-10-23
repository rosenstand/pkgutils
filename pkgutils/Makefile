CC=gcc -std=c99
CFLAGS=-DDEBUG -g -Wall -O2 -I./include
LDFLAGS=-larchive -lbz2 -lz

OBJS = list.o misc.o pkg_db.o libpkgadd.o libpkgrm.o filemode.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: pkgadd pkgrm pkginfo

pkgadd: $(OBJS) pkgadd.o
	$(CC) $(CFLAGS) $(OBJS) pkgadd.o $(LDFLAGS) -o $@

pkgrm: $(OBJS) pkgrm.o
	$(CC) $(CFLAGS) $(OBJS) pkgrm.o $(LDFLAGS) -o $@

pkginfo: $(OBJS) pkginfo.o
	$(CC) $(CFLAGS) $(OBJS) pkginfo.o $(LDFLAGS) -o $@

clean:
	-rm -f *.o pkgadd pkgrm pkginfo
