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
#include <pkgutils/types.h>
#include <archive.h>
#include <archive_entry.h>

typedef void (*do_archive_fun_t)(struct archive *ar, struct archive_entry *en,
                                 void *arg1, void *arg2);
void pkgutils_version();
int die(const char *str);
void *fmalloc(size_t size);
const char *base_filename(const char *name);

int pkg_cmp(const void *a, const void *b);
void intersect_uniq(void **a, size_t asz, void **b, size_t bsz,
                    int (*cmpf)(const void *a, const void *b),
                    void (*intef)(void **ai, void **bj, void *arg),
                    void (*uniqf)(void **ai, void *arg),
		    void *arg);
pkg_desc_t *pkg_find_pkg(const char *name);
int pkg_make_desc(const char *pkg_path, pkg_desc_t *pkg);
int do_archive(const char *pkg_path, do_archive_fun_t func, void *arg1,
               void *arg2);
int fetch_line_fields(char *line);

#ifdef DEBUG
#define dbg(str, ...) printf(str, __VA_ARGS__)
#else
#define dbg(str, ...)
#endif
