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
#include <string.h>
#include <pkgutils/pkgutils.h>
#include "entry.h"

int main(int argc, char *argv[]) {
	const char *name = base_filename(argv[0]);
	if (!strcmp(name, "pkgadd")) PKGADD_ENTRY(argc,argv);
	else if (!strcmp(name, "pkgrm")) PKGRM_ENTRY(argc,argv);
	else if (!strcmp(name, "pkginfo")) PKGINFO_ENTRY(argc,argv);
	else puts("This is a static all-in-one version of pkgadd, pkginfo "
	          "and pkgrm.\n"
	          "Invoke it with the name of the utility you want.");
	return 1;
}
