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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/param.h>

#include <pkgutils/pkgutils.h>

void pkg_lock_db() {
	char *dbdirpath;
	dbdirpath = fmalloc(strlen(opt_root) + sizeof(PKG_DB_DIR));
	strcpy(dbdirpath, opt_root);
	strcat(dbdirpath, PKG_DB_DIR);

	db_lock = open(dbdirpath, 0);
	if (db_lock < 0) die("Can't open " PKG_DB_DIR " for the lock.");
	if (flock(db_lock, LOCK_EX | LOCK_NB))
		die("database in use");
	free(dbdirpath);
	return;
}

int pkg_unlock_db() {
	if (flock(db_lock, LOCK_UN))
		die("can't unlock " PKG_DB_DIR);
	return close(db_lock);
}

void pkg_read_db(FILE *pkg_db_file) {
	char *line = fmalloc(MAXPATHLEN);
	size_t line_size;
	int cnt = 0;
	pkg_desc_t *pkg = NULL;
	pkg_file_t *file;
	while (fgets(line, MAXPATHLEN, pkg_db_file)) {
		cnt++;
		line_size = strlen(line) + 1;
		line[line_size-2] = '\0'; // chop \n
		
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

void pkg_free_db() {
	pkg_unlock_db();
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

void pkg_init_db() {
	FILE *pkg_db_file;
	char *dbpath;

	dbpath = fmalloc(strlen(opt_root) + sizeof(PKG_DB_DIR "db"));
	strcpy(dbpath, opt_root);
	strcat(dbpath, PKG_DB_DIR "db");

	pkg_lock_db();
	pkg_db_file = fopen(dbpath, "r");
	if (!pkg_db_file) die(dbpath);

	list_init(&pkg_db);
	pkg_read_db(pkg_db_file);

	if (fclose(pkg_db_file)) die("can't close db file");
	free(dbpath);
	return;
}

int pkg_commit_db() {
	size_t dbpath_size;
	char *dbpath;
	char *new_dbpath;
	FILE *new_dbfile;

	dbpath_size = strlen(opt_root) + sizeof(PKG_DB_DIR "db");
	dbpath = fmalloc(dbpath_size);
	strcpy(dbpath, opt_root);
	strcat(dbpath, PKG_DB_DIR "db");

	new_dbpath = fmalloc(dbpath_size + sizeof(".new") - 1);
	strcpy(new_dbpath, dbpath);
	strcat(new_dbpath, ".new");

	new_dbfile = fopen(new_dbpath, "w");
	if (!new_dbfile) die(new_dbpath);

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
		die("Can't overwrite old database with new one");
	free(dbpath);
	free(new_dbpath);
	return 0;
}
