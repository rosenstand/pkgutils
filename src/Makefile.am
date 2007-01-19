AM_CFLAGS		= -std=c99                             \
                          -DSYSCONFDIR=\"@sysconfdir@\"        \
                          -DLOCALSTATEDIR=\"@localstatedir@\"
INCLUDES                = -I$(top_srcdir)/include -include ../config.h
EXTRA_DIST              = entry.h

lib_LTLIBRARIES         = libpkg.la
libpkg_la_SOURCES       = list.c misc.c libpkgdb.c libpkgadd.c libpkgrm.c filemode.c
libpkg_la_LIBADD        = $(LIBARCHIVE)

bin_PROGRAMS            = pkgadd pkginfo pkgrm pkgutils
pkgadd_SOURCES          = pkgadd.c
pkgadd_LDADD            = -lpkg
pkginfo_SOURCES         = pkginfo.c
pkginfo_LDADD           = -lpkg
pkgrm_SOURCES           = pkgrm.c
pkgrm_LDADD             = -lpkg
pkgutils_SOURCES        = pkgadd.c pkginfo.c pkgrm.c pkgutils.c
pkgutils_CPPFLAGS       = -DSTATIC
pkgutils_LDADD          = -lpkg
pkgutils_LDFLAGS        = $(LDFLAGS) -all-static
