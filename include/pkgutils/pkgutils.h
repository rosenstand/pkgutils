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
#include <stdlib.h>
#include <pkgutils/list.h>
#include <pkgutils/types.h>
#include <pkgutils/misc.h>
#include <pkgutils/filemode.h>

#define PKG_EXT         ".pkg.tar.gz"

#define PKG_ADD_FORCE      1
#define PKG_ADD_FORCE_PERM 2

// global variables
extern char *opt_root;
extern list_t pkg_db;
extern int db_lock;

// database managing
extern void pkg_lock_db(void);
extern void pkg_unlock_db(void);
extern void pkg_init_db(void);
extern void pkg_free_db(void);
extern int pkg_commit_db(void);

// package management
extern int pkg_add(const char *pkg_path, int opts);
extern int pkg_rm(const char *pkg_name);
