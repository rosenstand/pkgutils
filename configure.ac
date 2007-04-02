AC_INIT([pkgutils-c], [1.4], [cbou@mail.ru])
AC_CONFIG_SRCDIR([src/pkgutils.c])
AM_INIT_AUTOMAKE

AC_LANG([C])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE
AC_HEADER_STDC
AM_CONFIG_HEADER(config.h)

CFLAGS="$CFLAGS -Wall -Wredundant-decls -Wnested-externs -Wstrict-prototypes \
-Wmissing-prototypes -Wpointer-arith -Winline -Wcast-align -Wbad-function-cast"

AC_CHECK_LIB([archive], [archive_read_open_FILE], [AC_CHECK_HEADER([archive.h], [LIBARCHIVE='-larchive'])])
if test -n "$LIBARCHIVE"; then
	CFLAGS="$CFLAGS"
	LDFLAGS="$LDFLAGS $LIBARCHIVE"
else
	AC_MSG_ERROR([libarchive >= 1.3 is needed to compile pkgutils]);
fi

AC_OUTPUT([
	Makefile
	etc/Makefile
	include/Makefile
	include/pkgutils/Makefile
	man/Makefile
	man/pkgadd.8
	man/pkginfo.8
	man/pkgmk.8
	man/pkgrm.8
	man/rejmerge.8
	scripts/Makefile
	scripts/pkgmk
	scripts/rejmerge
	src/Makefile
])
