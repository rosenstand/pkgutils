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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <pkgutils/pkgutils.h>
#include <pkgutils/filemode.h>

static
int opt_installed;
static
char *opt_list = NULL,
     *opt_footprint = NULL;

static
void print_usage(const char *argv0) {
	printf("Usage: %s [-ilofr]\n", argv0);
	puts("  -i  --installed           list installed packages\n"
	     "  -l  --list <package|file> list files for file or package\n"
	     "  -o  --owner <pattern>     print package owner\n"
	     "  -f  --footprint <file>    print footprint for <file>\n"
	     "  -r  --root                specify alternate root");
	return;
}

static
void parse_opts(int argc, char *argv[]) {
	char c;
	struct option opts[] = {
		{"root",         1, NULL, 'r'},
		{"installed",    0, NULL, 'i'},
		{"list"     ,    1, NULL, 'l'},
		{"footprint",    1, NULL, 'f'},
		{NULL,0,NULL,0}
	};

	while ((c = getopt_long(argc, argv, "r:il:f:", opts, NULL)) != -1) {
		switch (c) {
			case 'r':
				opt_root = optarg;
				break;
			case 'i':
				opt_installed = 1;
				break;
			case 'l':
				opt_list = optarg;
				break;
			case 'f':
				opt_footprint = optarg;
				break;
			case '?':
				exit(1);
				break;
			default:
				break;
		}
	}
	if (argc <= 1) {
		print_usage(argv[0]);
		exit(1);
	}
	return;
}

static
void print_footprint(struct archive *ar, struct archive_entry *en, void *unused1,
                   void *unused2) {
	static char mode[11];
	struct passwd *pw;
	struct group *gr;

	mode_string(archive_entry_mode(en), mode);
	printf("%s\t", mode);
	
	pw = getpwuid(archive_entry_uid(en));
	if (pw) printf("%s/", pw->pw_name);
	else printf("%d/", archive_entry_uid(en));

	gr = getgrgid(archive_entry_gid(en));
	if (gr) printf("%s\t", gr->gr_name);
	else printf("%d\t", archive_entry_gid(en));

	puts(archive_entry_pathname(en));
	return;
}

static
int footprint() {
	if (!strchr(opt_footprint, '#')) return 1;
	return do_archive(opt_footprint, print_footprint, NULL, NULL);
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
		ret = do_archive(opt_list, list_ar_files, NULL, NULL);
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
	                 "a package file.\n", opt_list);
	return ret;
}

int main(int argc, char *argv[]) {
	int ret = 0;
	opt_root = "/";
	parse_opts(argc, argv);
	if (opt_installed) {
		list_for_each(_pkg, &pkg_db) {
			pkg_desc_t *pkg = _pkg->data;
			printf("%s %s\n", pkg->name, pkg->version);
		}
	}
	else if (opt_list) {
		ret = list();
	}
	else if (opt_footprint) {
		ret = footprint();
	}
	else {
		ret = 1;
		print_usage(argv[0]);
	}
	exit(ret);
	return ret;
}