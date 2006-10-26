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

int comparr(const void *a, const void *b) {
	pkg_file_t *filea = (*(const list_entry_t **)a)->data;
	pkg_file_t *fileb = (*(const list_entry_t **)b)->data;
	return strcmp(filea->path, fileb->path);
}

// Delete from the list files which referenced by other packages.
// Thus they will not removed from the filesystem.
static
void delete_refs(pkg_desc_t *pkg2rm) {
	size_t dbsize = 0, cnt;
	list_entry_t **dbfiles, **pkgfiles;

	// creating db files sorted array
	list_for_each(_dbpkg, &pkg_db) {
		pkg_desc_t *dbpkg = _dbpkg->data;
		if (dbpkg == pkg2rm) continue;
		dbsize += dbpkg->files.size;
	}
	printf("size: %d\n", dbsize);

	dbfiles = fmalloc(dbsize * sizeof(void*));
	cnt = 0;
	list_for_each(_dbpkg, &pkg_db) {
		pkg_desc_t *dbpkg = _dbpkg->data;
		if (dbpkg == pkg2rm) continue;
		list_for_each(_dbfile, &dbpkg->files) {
			dbfiles[cnt] = _dbfile;
			cnt++;
		}
	}
	qsort(dbfiles, dbsize, sizeof(void*), comparr);

	// creating pkg files sorted array
	pkgfiles = fmalloc(pkg2rm->files.size * sizeof(void*));
	cnt = 0;
	list_for_each(_file, &pkg2rm->files) {
		pkgfiles[cnt] = _file;
		cnt++;
	}
	qsort(pkgfiles, pkg2rm->files.size, sizeof(void*), comparr);

	// find intersection
	size_t next = 0;
	for (int i = 0; i < pkg2rm->files.size; i++) {
		pkg_file_t *pkgfile = pkgfiles[i]->data;
		for (size_t j = next; j < dbsize; j++) {
			pkg_file_t *dbfile = dbfiles[j]->data;
			int cmp = strcmp(dbfile->path, pkgfile->path);
			if (cmp < 0) continue;
			else if (cmp > 0) break;
			else {
				next = j+1;
				while (next < dbsize) {
					pkg_file_t *dbfile2;
					dbfile2 = dbfiles[next]->data;
					if (strcmp(dbfile->path,
					           dbfile2->path)) break;
					next++;
				}
				puts(pkgfile->path);
				break;
			}
		}
	}

		/*list_for_each(_dbfile, &dbpkg->files) {
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
		}*/

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
	return 1;
	remove_from_fs(pkg2rm);

	free(pkg2rm->name);
	free(pkg2rm->version);
	free(pkg2rm);
	list_delete(&pkg_db, _pkg2rm);
	pkg_commit_db();

	return 0;

}
