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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/param.h>
#include <pkgutils/pkgutils.h>

#define PKG_DB_DIR   LOCALSTATEDIR"/lib/pkg"
#define PKG_DB_FILE  PKG_DB_DIR"/db"

char *opt_root;
list_t pkg_db;
int db_lock;

void pkg_lock_db(void) {
	char *dbdirpath;

	dbdirpath = fmalloc(strlen(opt_root) + sizeof(PKG_DB_DIR));
	strcpy(dbdirpath, opt_root);
	strcat(dbdirpath, PKG_DB_DIR);

	db_lock = open(dbdirpath, 0);
	if (db_lock < 0) die(dbdirpath);
	if (flock(db_lock, LOCK_EX | LOCK_NB)) die(dbdirpath);
	free(dbdirpath);
	return;
}

void pkg_unlock_db(void) {
	if (flock(db_lock, LOCK_UN)) die("Can't unlock database");
	if (close(db_lock)) die("Can't close database directory");
	return;
}

static
void pkg_read_db(FILE *pkg_db_file) {
	char *line = fmalloc(MAXPATHLEN+1);
	size_t line_size;
	int cnt = 0;
	pkg_desc_t *pkg = NULL;
	pkg_file_t *file;
	while (fgets(line, MAXPATHLEN+1, pkg_db_file)) {
		cnt++;
		line_size = strlen(line) + 1;
		if (line[line_size-2] == '\n') line[line_size-2] = '\0';

		if (line[0] == '\0') {
			cnt = 0;
			list_append(&pkg_db, pkg);
			continue;
		}

		switch (cnt) {
			case 1:
				pkg = fmalloc(sizeof(pkg_desc_t));
				list_init(&pkg->files);
				pkg->name = fmalloc(line_size);
				strcpy(pkg->name, line);
				break;
			case 2:
				pkg->version = fmalloc(line_size);
				strcpy(pkg->version, line);
				break;
			default:
				file = fmalloc(sizeof(pkg_file_t));
				file->pkg = pkg;
				file->conflict = CONFLICT_NONE;
				file->path = fmalloc(line_size);
				strcpy(file->path, line);
				line_size = strlen(file->path) + 1;
				if (file->path[line_size-2] == '/') {
					file->mode = S_IFDIR;
					file->path[line_size-2] = '\0';
				}
				else file->mode = 0;
				list_append(&pkg->files, file);
				break;
		}
	}
	free(line);
	return;
}

void pkg_free_db(void) {
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		list_for_each(_file, &pkg->files) {
			pkg_file_t *file = _file->data;
			free(file->path);
			free(file);
		}
		free(pkg->name);
		free(pkg->version);
		list_free(&pkg->files);
		free(pkg);
	}
	list_free(&pkg_db);
	return;
}

void pkg_init_db(void) {
	FILE *pkg_db_file;
	char *dbpath;

	dbpath = fmalloc(strlen(opt_root) + sizeof(PKG_DB_FILE));
	strcpy(dbpath, opt_root);
	strcat(dbpath, PKG_DB_FILE);

	pkg_db_file = fopen(dbpath, "r");
	if (!pkg_db_file) die(dbpath);

	list_init(&pkg_db);
	pkg_read_db(pkg_db_file);

	if (fclose(pkg_db_file)) die("Can't close database");
	free(dbpath);
	return;
}

static
void sort_db(void) {
	void **pkgs;
	int i;

	pkgs = fmalloc(sizeof(void*) * pkg_db.size);

	i = 0;
	list_for_each(_pkg, &pkg_db) pkgs[i++] = _pkg->data;

	qsort(pkgs, pkg_db.size, sizeof(void*), pkg_cmp);

	i = 0;
	list_for_each(_pkg, &pkg_db) _pkg->data = pkgs[i++];

	free(pkgs);
	return;
}

int pkg_commit_db(void) {
	size_t dbpath_size;
	char *dbpath;
	char *new_dbpath;
	FILE *new_dbfile;

	dbpath_size = strlen(opt_root) + sizeof(PKG_DB_FILE);
	dbpath = fmalloc(dbpath_size);
	strcpy(dbpath, opt_root);
	strcat(dbpath, PKG_DB_FILE);

	new_dbpath = fmalloc(dbpath_size + sizeof(".new") - 1);
	strcpy(new_dbpath, dbpath);
	strcat(new_dbpath, ".new");

	new_dbfile = fopen(new_dbpath, "w");
	if (!new_dbfile) die(new_dbpath);

	sort_db();
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		fputs(pkg->name, new_dbfile);
		fputc('\n', new_dbfile);
		fputs(pkg->version, new_dbfile);
		fputc('\n', new_dbfile);

		list_for_each(_file, &pkg->files) {
			pkg_file_t *file = _file->data;
			fputs(file->path, new_dbfile);
			if (S_ISDIR(file->mode)) fputc('/', new_dbfile);
			fputc('\n', new_dbfile);
		}
		fputc('\n', new_dbfile);
	}
	fflush(new_dbfile);
	fsync(fileno(new_dbfile));
	fclose(new_dbfile);

	if (rename(new_dbpath, dbpath))
		die("Can't replace old database");
	free(dbpath);
	free(new_dbpath);
	return 0;
}
