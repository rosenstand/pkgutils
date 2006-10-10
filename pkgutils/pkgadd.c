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
#include <pkgutils/pkgutils.h>

static
int opt_force;

static
void print_usage(const char *argv0) {
	printf("Usage: %s [-rfp] <package>\n", argv0);
	puts("  -r  --root         specify alternate root\n"
	     "  -f  --force        ignore database and filesystem conflicts\n"
	     "  -p  --force-perms  ignore permissions conflicts");
	return;
}

static
void parse_opts(int argc, char *argv[]) {
	char c;
	struct option opts[] = {
		{"root",         1, NULL, 'r'},
		{"force",        0, NULL, 'f'},
		{"force-perms",  0, NULL, 'p'},
		{NULL,0,NULL,0}
	};

	while ((c = getopt_long(argc, argv, "r:fp", opts, NULL)) != -1) {
		switch (c) {
			case 'r':
				opt_root = optarg;
				break;
			case 'f':
				opt_force |= PKG_ADD_FORCE;
				break;
			case 'p':
				opt_force |= PKG_ADD_FORCE_PERM;
				break;
			case '?':
				exit(1);
				break;
			default:
				break;
		}
	}

	if (optind >= argc) {
		print_usage(argv[0]);
		exit(1);
	}

	return;
}

int main(int argc, char *argv[]) {
	int found_conflicts;

	opt_root = "/";
	parse_opts(argc, argv);
	pkg_init_db();
	while (optind < argc) {
		found_conflicts = pkg_add(argv[optind], opt_force);
		if (found_conflicts & CONFLICT_PERM &&
				!(opt_force & PKG_ADD_FORCE_PERM)) exit(1);
		else if (found_conflicts & ~CONFLICT_PERM &&
					!(opt_force & PKG_ADD_FORCE)) exit(1);
		optind++;
	}
	pkg_free_db();
	exit(0);
	return 0;
}
