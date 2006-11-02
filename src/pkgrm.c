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
void print_usage(const char *argv0) {
	printf("Usage: %s [-rhv] <package>\n", argv0);
	puts("  -r  --root      specify alternate root\n"
	     "  -h  --help      display this help\n"
	     "  -v  --version   display version information");
	return;
}

static
void parse_opts(int argc, char *argv[]) {
	char c;
	struct option opts[] = {
		{"root"   , 1, NULL, 'r'},
		{"help"   , 0, NULL, 'h'},
		{"version", 0, NULL, 'v'},
		{NULL  , 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "r:hv", opts, NULL)) != -1) {
		switch (c) {
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

int PKGRM_ENTRY(int argc, char *argv[]) {
	opt_root = "/";
	parse_opts(argc, argv);
	pkg_init_db();
	while (optind < argc) {
		pkg_rm(argv[optind]);
		optind++;
	}
	pkg_free_db();
	exit(0);
	return 0;
}
