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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pkgutils/pkgutils.h>

void pkgutils_version() {
	puts("pkgutils-c "VERSION" (C rewrite)\n\n"
	     "Copyright (C) 2006  Anton Vorontsov\n"
	     "This is free software.  You may redistribute copies of it \n"
	     "under the terms of the GNU General Public License \n"
	     "<http://www.gnu.org/licenses/gpl.html>.\n"
	     "There is NO WARRANTY, to the extent permitted by law.\n"
	     "Original pkgutils:\n"
	     "Copyright (C) 2000-2005 by Per Liden <per@fukt.bth.se>");
	return;
}

int die(const char *str) {
	fprintf(stderr, "%s: %s\n", str, strerror(errno));
	abort();
	return 1;
}

static
void malloc_failed() {
	fputs("Out of memory", stderr);
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

int pkg_cmp(const void *a, const void *b) {
	pkg_desc_t *pkga = *(pkg_desc_t**)a;
	pkg_desc_t *pkgb = *(pkg_desc_t**)b;
	return strcmp(pkga->name, pkgb->name);
}

int file_cmp(const void *a, const void *b) {
	pkg_file_t *filea = (*(const list_entry_t **)a)->data;
	pkg_file_t *fileb = (*(const list_entry_t **)b)->data;
	return strcmp(filea->path, fileb->path);
}

// find intersection
void intersect_uniq(void **a, size_t asz, void **b, size_t bsz,
                    int (*cmpf)(const void *a, const void *b),
                    void (*intef)(void **ai, void **bj, void *arg),
                    void (*uniqf)(void **ai, void *arg),
		    void *arg) {
	size_t next = 0;
	int cmp = 1;
	for (int i = 0; i < asz; i++) {
		for (size_t j = next; j < bsz; j++) {
			cmp = cmpf(&b[j], &a[i]);
			if (cmp < 0) continue;
			else if (cmp > 0) break;
			else {
				next = j+1;
				while (next < bsz) {
					if (cmpf(&b[j], &b[next])) break;
					next++;
				}
				if (intef) intef(&a[i], &b[j], arg);
				break;
			}
		}
		if (cmp != 0 && uniqf) uniqf(&a[i], arg);
	}
	return;
}

pkg_desc_t *pkg_find_pkg(const char *name) {
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		if (strcmp(pkg->name, name)) continue;
		return pkg;
	}
	return NULL;
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
	
	// a#bPKG_EXT is minimal
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
	return err;
}

// Calls func for the every archive entry. Handy function, and also eliminates
// code duplication. Returns 0 if succeeded.
int do_archive(FILE *pkg, do_archive_fun_t func, void *arg1, void *arg2) {
	struct archive *ar;
	struct archive_entry *en;
	int err = 0;

	fseek(pkg, 0L, SEEK_SET);
	ar = archive_read_new();
	if (!ar) malloc_failed();
	archive_read_support_compression_gzip(ar);
	archive_read_support_format_tar(ar);
	if (archive_read_open_FILE(ar, pkg) != ARCHIVE_OK) {
		puts(archive_error_string(ar));
		err = -1;
		goto failed;
	}
	
	while (1) {
		err = archive_read_next_header(ar, &en);
		if (err == ARCHIVE_OK) {
			func(ar, en, arg1, arg2);
		}
		else if (err == ARCHIVE_EOF) {
			err = 0;
			break;
		}
		else {
			puts(archive_error_string(ar));
			err = -1;
			goto failed;
		}
	}

failed:
	archive_read_finish(ar);
	return err;
}

int do_archive_once(const char *fname, do_archive_fun_t func, void *arg1,
                    void *arg2) {
	FILE *pkgf;
	int err;
	pkgf = fopen(fname, "r");
	if (!pkgf) {
		fprintf(stderr, "Can't open %s: %s\n", fname,
		        strerror(errno));
		return -1;
	}
	err = do_archive(pkgf, func, arg1, arg2);
	fclose(pkgf);
	return err;
}

// This function transforms string
// from: "abc \"def ghi\""
//   to: "abc\0def ghi\0"
// Returns count of fields fetched (2 in that example), -1 on parse error.
int fetch_line_fields(char *line) {
	size_t i = 0, j = 0, n = 0;
	int esc = 0, quo = 0, skip = 0;
	
	while (1) {
		switch (line[i]) {
			case '\0':
			case '\n':
				if (quo) return -1;
			case '#':
				if (quo) break;
				line[j] = '\0';
				return n;
				break;
			case ' ':
			case '\t':
				if (quo) break;
				skip = 1;

				if (line[i+1] != ' ' && line[i+1] != '\t' &&
				    line[i+1] != '\n' && line[i+1] != '#') {
					n++;
					line[j] = '\0';
					j++;
				}
				break;
			case '\"':
				if (quo) {
					if (!esc) {
						quo = 0;
						skip = 1;
					}
					else esc = 0;
				}
				else {
					quo = 1;
					skip = 1;
				}
				break;
			case '\\':
				if (!quo) return -1;

				if (esc) esc = 0;
				else {
					esc = 1;
					skip = 1;
				}
				break;
			default:
				if (!n) n = 1;
				esc = 0;
				break;
		}

		if (skip) {
			skip = 0;
			i++;
			continue;
		}

		line[j] = line[i];
		j++;
		i++;
	}
	return 0;
}
