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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pkgutils/pkgutils.h>
#include "entry.h"

static
int opt_force;

static
void print_usage(const char *argv0) {
	printf("Usage: %s [-opfrhv] <package>\n", argv0);
	puts("  -o  --force-over    ignore database and filesystem conflicts\n"
	     "  -p  --force-perms   ignore permissions conflicts\n"
	     "  -f  --force         same as -o and -p together\n"
	     "  -r  --root          specify alternate root\n"
	     "  -h  --help          display this help\n"
	     "  -v  --version       display version information");
	return;
}

static
void parse_opts(int argc, char *argv[]) {
	int c;
	struct option opts[] = {
		{"force-over" , 0, NULL, 'o'},
		{"force-perms", 0, NULL, 'p'},
		{"force"      , 0, NULL, 'f'},
		{"upgrade"    , 1, NULL, 'u'},
		{"root"       , 1, NULL, 'r'},
		{"help"       , 0, NULL, 'h'},
		{"version"    , 0, NULL, 'v'},
		{NULL         , 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "opfur:hv", opts, NULL)) != -1) {
		switch (c) {
			case 'f': opt_force |= PKG_ADD_FORCE_PERM;
			case 'o': opt_force |= PKG_ADD_FORCE; break;
			case 'p': opt_force |= PKG_ADD_FORCE_PERM; break;
			case 'u': break; // compatibility with C++ish pkgutils
			case 'r': opt_root = optarg; break;
			case 'h': print_usage(argv[0]); exit(0); break;
			case 'v': pkgutils_version(); exit(0); break;
			case '?': exit(1); break;
			default: break;
		}
	}

	if (optind >= argc) {
		print_usage(argv[0]);
		exit(1);
	}

	return;
}

int PKGADD_ENTRY(int argc, char *argv[]) {
	int found_conflicts;

	opt_root = "";
	parse_opts(argc, argv);

	pkg_lock_db();
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
	pkg_unlock_db();

	exit(0);
	return 0;
}
