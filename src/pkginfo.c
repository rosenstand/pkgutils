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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <regex.h>
#include <pkgutils/pkgutils.h>
#include "entry.h"

static
int opt_installed;
static
char *opt_list = NULL,
     *opt_owner = NULL,
     *opt_footprint = NULL;

static
void print_usage(const char *argv0) {
	printf("Usage: %s [-ilofrhv]\n", argv0);
	puts("  -i  --installed           list installed packages\n"
	     "  -l  --list <package|file> list files for file or package\n"
	     "  -o  --owner <pattern>     print package owner\n"
	     "  -f  --footprint <file>    print footprint for <file>\n"
	     "  -r  --root                specify alternate root\n"
	     "  -h  --help                display this help\n"
	     "  -v  --version             display version information");
	return;
}

static
void parse_opts(int argc, char *argv[]) {
	int c;
	struct option opts[] = {
		{"installed",    0, NULL, 'i'},
		{"list"     ,    1, NULL, 'l'},
		{"owner"    ,    1, NULL, 'o'},
		{"footprint",    1, NULL, 'f'},
		{"root"     ,    1, NULL, 'r'},
		{"help"     ,    0, NULL, 'h'},
		{"version"  ,    0, NULL, 'v'},
		{NULL       ,    0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv,"il:o:f:r:hv", opts, NULL)) != -1) {
		switch (c) {
			case 'i': opt_installed = 1; break;
			case 'l': opt_list = optarg; break;
			case 'o': opt_owner = optarg; break;
			case 'f': opt_footprint = optarg; break;
			case 'r': opt_root = optarg; break;
			case 'h': print_usage(argv[0]); exit(0); break;
			case 'v': pkgutils_version(); exit(0); break;
			case '?': exit(1); break;
			default: break;
		}
	}
	if (argc <= 1) {
		print_usage(argv[0]);
		exit(1);
	}
	return;
}

static
void print_footprint(struct archive *ar, struct archive_entry *en,
                     void *unused1, void *unused2) {
	struct stat st;
	static char smode[11];
	struct passwd *pw;
	struct group *gr;
	
	// XXX: archive_entry_copy_stat() seems buggy, fill stat manually
	st.st_mode = archive_entry_mode(en);
	st.st_uid = archive_entry_uid(en);
	st.st_gid = archive_entry_gid(en);
	st.st_size = archive_entry_size(en);
	// XXX: another workaround for the libarchive possible bug: it do
	//      not set any type to the hardlinks, but should.
	if (archive_entry_hardlink(en)) {
		st.st_mode = st.st_mode |= S_IFREG;
		// XXX: libarchive do not set size for the hardlinks,
		// thus currently we can't handle empty hardlinks :-(
		st.st_size = 1;
	}

	printf("%s\t", mode_string(st.st_mode, smode));

	pw = getpwuid(st.st_uid);
	if (pw) printf("%s/", pw->pw_name);
	else printf("%d/", st.st_uid);

	gr = getgrgid(st.st_gid);
	if (gr) printf("%s\t", gr->gr_name);
	else printf("%d\t", st.st_gid);

	printf("%s", archive_entry_pathname(en));

	if (S_ISLNK(st.st_mode)) {
		printf(" -> %s", archive_entry_symlink(en));
	}
	else if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
		printf(" (%d, %d)", (int)archive_entry_rdevmajor(en),
		                    (int)archive_entry_rdevminor(en));
	}
	else if (S_ISREG(st.st_mode) && !st.st_size) {
		printf(" (EMPTY)");
	}

	puts("");
	return;
}

static
int footprint() {
	if (!strchr(opt_footprint, '#')) return 1;
	return do_archive_once(opt_footprint, print_footprint, NULL, NULL);
}

static
int owner() {
	int ret = 1;
	size_t width = 0;
	regex_t re;
	list_t files;
	
	if (opt_owner[0] == '/') strcpy(opt_owner, opt_owner+1);

	if (regcomp(&re, opt_owner, REG_EXTENDED | REG_NOSUB)) {
		fputs("Failed to compile regular expression\n", stderr);
		return 1;
	}
	
	list_init(&files);
	pkg_init_db();
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		list_for_each(_file, &pkg->files) {
			pkg_file_t *file = _file->data;
			if (regexec(&re, file->path, 0, 0, 0)) continue;
			list_append(&files, file);
			width = MAX(strlen(pkg->name), width);
		}
	}
	
	printf("%-*s %s\n", width, "Package", "File");
	list_for_each(_file, &files) {
		pkg_file_t *file = _file->data;
		printf("%-*s %s\n", width, file->pkg->name, file->path);
	}

	list_free(&files);
	pkg_free_db();
	return ret;
}

static
void list_ar_files(struct archive *ar, struct archive_entry *en, void *unused1,
                   void *unused2) {
	puts(archive_entry_pathname(en));
	return;
}

static
int list() {
	int ret = 1;
	pkg_desc_t *pkg;

	if (strchr(opt_list, '#')) {
		ret = do_archive_once(opt_list, list_ar_files, NULL, NULL);
		return ret;
	}

	pkg_init_db();
	if ((pkg = pkg_find_pkg(opt_list)) != NULL) {
		ret = 0;
		list_for_each(_file, &pkg->files) {
			pkg_file_t *file = _file->data;
			printf("%s", file->path);
			S_ISDIR(file->mode) ? puts("/") : puts("");
		}
	}
	pkg_free_db();
	if (ret) fprintf(stderr, "\"%s\" is neither an installed package nor "
	                 "a package archive\n", opt_list);
	return ret;
}

static
int installed() {
	pkg_init_db();
	
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		printf("%s %s\n", pkg->name, pkg->version);
	}

	pkg_free_db();
	return 0;
}

int PKGINFO_ENTRY(int argc, char *argv[]) {
	int ret = 1;
	opt_root = NULL;
	parse_opts(argc, argv);
	opt_root = check_and_alloc_root(opt_root);
	
	if (opt_installed) ret = installed();
	else if (opt_list) ret = list();
	else if (opt_owner) ret = owner();
	else if (opt_footprint) ret = footprint();
	else print_usage(argv[0]);

	free(opt_root);
	exit(ret);
	return ret;
}
