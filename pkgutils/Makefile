CC=gcc -std=c99
CFLAGS=-DDEBUG -g -Wall -O2 -I./include
LDFLAGS=-larchive -lbz2 -lz

OBJS = list.o misc.o pkg_db.o libpkgadd.o libpkgrm.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: pkgrm pkgadd

pkgadd: $(OBJS) pkgadd.o
	$(CC) $(CFLAGS) $(OBJS) pkgadd.o $(LDFLAGS) -o $@

pkgrm: $(OBJS) pkgrm.o
	$(CC) $(CFLAGS) $(OBJS) pkgrm.o $(LDFLAGS) -o $@

clean:
	-rm -f *.o pkgadd pkgrm
