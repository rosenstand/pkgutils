AC_INIT([pkgutils], [1.2], [cbou@mail.ru])
AC_CONFIG_SRCDIR([src/pkgutils.c])
AM_INIT_AUTOMAKE

AC_LANG([C])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE
AC_HEADER_STDC

AC_CHECK_LIB([archive], [archive_read_data], [AC_CHECK_HEADER([archive.h], [LIBARCHIVE='-larchive'])])
if test -n "$LIBARCHIVE"; then
	CFLAGS="$CFLAGS"
	LDFLAGS="$LDFLAGS $LIBARCHIVE"
else
	AC_MSG_ERROR([libarchive is needed to compile pkgutils]);
fi

sysconfdir="/etc"
localstatedir="/var/lib"

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
