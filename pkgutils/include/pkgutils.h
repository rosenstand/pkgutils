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

#pragma once
#include <sys/types.h>
#include <stdlib.h>
#include <pkgutils/list.h>
#include <pkgutils/types.h>
#include <pkgutils/misc.h>

#define PKG_EXT         ".pkg.tar.gz"
#define PKG_DB_DIR      "var/lib/pkg/"
#define PKG_REJECT_DIR  "var/lib/pkg/rejected/"
#define PKG_ADD_CONFIG  "usr/etc/pkgadd.conf"

#define PKG_ADD_FORCE      1
#define PKG_ADD_FORCE_PERM 2

// global variables
char *opt_root;
list_t pkg_db;
int db_lock;

// database managing
void pkg_init_db();
void pkg_free_db();
int pkg_commit_db();

// package management
int pkg_add(const char *pkg_path, int opts);
int pkg_rm(const char *pkg_name);
