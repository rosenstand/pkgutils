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
#include <string.h>
#include <errno.h>
#include <sys/param.h>

#include <pkgutils/pkgutils.h>

// Delete from the list files which referenced by other packages.
// Thus they will not removed from the filesystem.
static
void delete_refs(pkg_desc_t *pkg2rm) {
	list_for_each(_dbpkg, &pkg_db) {
		pkg_desc_t *dbpkg = _dbpkg->data;
		if (dbpkg == pkg2rm) continue;
		list_for_each(_dbfile, &dbpkg->files) {
			pkg_file_t *dbfile = _dbfile->data;
			list_for_each(_file2rm, &pkg2rm->files) {
				pkg_file_t *file2rm = _file2rm->data;
				if (!strcmp(dbfile->path, file2rm->path)) {
					free(file2rm->path);
					free(file2rm);
					_file2rm = _file2rm->prev;
					list_delete(&pkg2rm->files,
					            _file2rm->next);
				}
			}
		}
	}
	return;
}

// unlink files from the filesystem
static
void remove_from_fs(pkg_desc_t *pkg2rm) {
	char *tmp = fmalloc(MAXPATHLEN);
	size_t root_len = strlen(opt_root);
	strcpy(tmp, opt_root);

	list_for_each_r(_file2rm, &pkg2rm->files) {
		pkg_file_t *file2rm = _file2rm->data;
		tmp[root_len] = '\0';
		strcat(tmp, file2rm->path);
		if (remove(tmp))
			fprintf(stderr, "can't remove %s: %s\n", tmp,
			        strerror(errno));
		free(file2rm->path);
		free(file2rm);
	}
	list_free(&pkg2rm->files);
	free(tmp);
}

int pkg_rm(const char *pkg_name) {
	pkg_desc_t *pkg2rm = NULL;
	list_entry_t *_pkg2rm = NULL;
	
	list_for_each(_dbpkg, &pkg_db) {
		pkg_desc_t *dbpkg = _dbpkg->data;
		if (strcmp(dbpkg->name, pkg_name)) continue;
		_pkg2rm = _dbpkg;
		pkg2rm = dbpkg;
		break;
	}

	if (!pkg2rm) {
		fprintf(stderr, "Package \"%s\" not found.\n", pkg_name);
		return -1;
	}

	delete_refs(pkg2rm);
	remove_from_fs(pkg2rm);

	free(pkg2rm->name);
	free(pkg2rm->version);
	free(pkg2rm);
	list_delete(&pkg_db, _pkg2rm);
	pkg_commit_db();

	return 0;

}
