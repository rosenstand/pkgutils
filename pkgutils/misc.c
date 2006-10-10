//  Original pkgutils:
//  Copyright (c) 2000-2005 Per Liden
//
//  That C-rewrite:
//  Copyright (c) 2006 Anton Vorontsov <cbou@mail.ru>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
//  USA.
//

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pkgutils/pkgutils.h>

int die(const char *str) {
	printf("%s: ", str);
	puts(strerror(errno));
	abort();
	return 1;
}

void malloc_failed() {
	puts("malloc failed. out of memory");
	abort();
	return;
}

void *fmalloc(size_t size) {
	void *tmp = malloc(size);
	if (!tmp) malloc_failed();
	return tmp;
}

const char *base_filename(const char *name) {
	char *slash = strrchr(name, '/');
	if (slash) name = slash+1;
	return name;
}

// fills pkg_desk_t structure using package's file name
// normally returns 0, and -1 if path does not point to a proper
// package name
int pkg_make_desc(const char *pkg_path, pkg_desc_t *pkg) {
	const char *fname;
	char *parsed, *tmp;
	size_t fname_len;
	int err = 0;

	fname = base_filename(pkg_path);
	fname_len = strlen(fname);

	if (fname_len < sizeof(PKG_EXT) - 1 + 3) return -1;

	parsed = fmalloc(strlen(fname) + 1);

	strcpy(parsed, fname);
	parsed[fname_len - (sizeof(PKG_EXT) - 1)] = '\0';
	
	pkg->name    = strtok_r(parsed, "#", &tmp);
	pkg->version = strtok_r(NULL,   "#", &tmp);
	if (!pkg->name || !pkg->version) {
		err = 1;
		goto failed;
	}

	tmp = fmalloc(strlen(pkg->name) + 1);
	strcpy(tmp, pkg->name);
	pkg->name = tmp;

	tmp = fmalloc(strlen(pkg->version) + 1);
	strcpy(tmp, pkg->version);
	pkg->version = tmp;
failed:
	free(parsed);
	return 0;
}
