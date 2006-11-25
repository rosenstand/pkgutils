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
#include <sys/types.h>

typedef enum {
	CONFLICT_NONE = 0,  // gag
	CONFLICT_SELF = 1,  // pseudo conflict when upgrading
	CONFLICT_DB   = 2,  // file conflict in database
	CONFLICT_FS   = 4,  // file conflict in filesystem
	CONFLICT_PERM = 8,  // mode/ownership conflict
	CONFLICT_REF  = 16  // reference conflict
} pkg_conflict_type_t;

typedef struct {
	char *name;
	char *version;
	list_t files;  // pkg_files_t list
} pkg_desc_t;

typedef struct {
	pkg_desc_t *pkg;
	pkg_conflict_type_t conflict;
	char *path;
	mode_t mode;
	uid_t uid;
	gid_t gid;
} pkg_file_t;
