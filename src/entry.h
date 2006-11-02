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

#pragma once

#ifdef STATIC
	#define PKGADD_ENTRY  pkgadd_main
	#define PKGRM_ENTRY   pkgrm_main
	#define PKGINFO_ENTRY pkginfo_main
	int PKGADD_ENTRY(int argc, char *argv[]);
	int PKGRM_ENTRY(int argc, char *argv[]);
	int PKGINFO_ENTRY(int argc, char *argv[]);
#else
	#define PKGADD_ENTRY  main
	#define PKGRM_ENTRY   main
	#define PKGINFO_ENTRY main
#endif
