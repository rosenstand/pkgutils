/*
 This file was a part of GNU coreutils, a bit modified for pkgutils.
 Copyright (C) Anton Vorontsov <cbou@mail.ru>

 filemode.c -- make a string describing file modes
 Copyright (C) 1985, 1990, 1993, 1998-2000, 2004 Free Software Foundation, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>

// Set the 's' and 't' flags in file attributes string CHARS,
// according to the file mode BITS.
static
void setst (mode_t bits, char *chars) {
	if (bits & S_ISUID) {
		/* Set-uid, but not executable by owner? */
		if (chars[3] != 'x') chars[3] = 'S';
		else chars[3] = 's';
	}
	if (bits & S_ISGID) {
		/* Set-gid, but not executable by group? */
		if (chars[6] != 'x') chars[6] = 'S';
		else chars[6] = 's';
	}
	if (bits & S_ISVTX) {
		/* Sticky, but not executable by others? */
		if (chars[9] != 'x') chars[9] = 'T';
		else chars[9] = 't';
	}
}

// Return a character indicating the type of file described by
// file mode BITS.
static
char ftypelet (mode_t bits) {
	if (S_ISBLK(bits))  return 'b';
	if (S_ISCHR(bits))  return 'c';
	if (S_ISDIR(bits))  return 'd';
	if (S_ISREG(bits))  return '-';
	if (S_ISLNK(bits))  return 'l';
	if (S_ISFIFO(bits)) return 'p';
	if (S_ISSOCK(bits)) return 's';
	return '?';
}

// mode_string - fill in string STR with an ls-style ASCII
// representation of the mode_t. 11 characters are stored in STR;
// terminating null is added.
char *mode_string (mode_t mode, char *str) {
	str[0] = ftypelet (mode);
	str[1] = mode & S_IRUSR ? 'r' : '-';
	str[2] = mode & S_IWUSR ? 'w' : '-';
	str[3] = mode & S_IXUSR ? 'x' : '-';
	str[4] = mode & S_IRGRP ? 'r' : '-';
	str[5] = mode & S_IWGRP ? 'w' : '-';
	str[6] = mode & S_IXGRP ? 'x' : '-';
	str[7] = mode & S_IROTH ? 'r' : '-';
	str[8] = mode & S_IWOTH ? 'w' : '-';
	str[9] = mode & S_IXOTH ? 'x' : '-';
	setst (mode, str);
	str[10] = '\0';
	return str;
}
