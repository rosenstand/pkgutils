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
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>
#include <sys/param.h>
#include <sys/file.h>
#include <assert.h>
#include <pkgutils/pkgutils.h>

static
list_t config_rules;

typedef enum {
	UPGRADE,
	INSTALL
} rule_type_t;

typedef struct {
	rule_type_t type;
	regex_t regex;
	uint8_t yes;
} rule_t;

static
void cleanup_config() {
	list_for_each(_rule, &config_rules) {
		rule_t *rule = _rule->data;
		regfree(&rule->regex);
		free(rule);
	}
	list_free(&config_rules);
	return;
}

static
void read_config() {
	char *config;
	FILE *f;
	char line[MAXPATHLEN+1];
	char *pline;
	size_t lineno = 0;
	int tmp;
	rule_t tmprule;
	
	list_init(&config_rules);
	config = fmalloc(strlen(opt_root) + sizeof(PKG_ADD_CONFIG));
	strcpy(config, opt_root);
	strcat(config, PKG_ADD_CONFIG);
	f = fopen(config, "r");
	if (!f) {
		free(config);
		return;
	}

	while (1) {
		if (!fgets(line, MAXPATHLEN+1, f)) break;
		lineno++;

		tmp = fetch_line_fields(line);
		if (tmp < 0) {
			fprintf(stderr, "%s: parse error at line %d\n", config,
			                                               lineno);
			abort();
		}

		if (tmp == 3 && !strcmp(line, "UPGRADE"))
			tmprule.type = UPGRADE;
		else if (tmp == 3 && !strcmp(line, "INSTALL"))
			tmprule.type = INSTALL;
		else if (tmp == 0)
			continue;
		else {
			fprintf(stderr, "%s: unknown rule: %s\n", config,
			                                          line);
			abort();
		}

		pline = line + strlen(line) + 1;
		if (regcomp(&tmprule.regex, pline, REG_EXTENDED | REG_NOSUB)) {
			fprintf(stderr, "%s: %s: invalid regex\n",
			        config, pline);
			abort();
		}

		pline = pline + strlen(pline) + 1;
		if (pline[0] == 'Y') tmprule.yes = 1;
		else tmprule.yes = 0;

		rule_t *rule = fmalloc(sizeof(rule_t));
		*rule = tmprule;
		list_append(&config_rules, rule);
	}
	
	free(config);
	fclose(f);
	return;
}

static
int report_conflicts(pkg_desc_t *pkg) {
	int types = CONFLICT_NONE;
	pkg_file_t *file;

	list_for_each(_file, &pkg->files) {
		file = _file->data;
		switch (file->conflict) {
			case CONFLICT_DB: 
			case CONFLICT_FS:
			case CONFLICT_PERM:
				types |= file->conflict; break;
			default: break;
		}
		
	}

	if (types & CONFLICT_DB) {
		puts("Files already registered in database:");
		list_for_each(_file, &pkg->files) {
			file = _file->data;
			if (file->conflict == CONFLICT_DB) puts(file->path);
		}
		puts("");
	}
	if (types & CONFLICT_FS) {
		puts("Files already present on filesystem:");
		list_for_each(_file, &pkg->files) {
			file = _file->data;
			if (file->conflict == CONFLICT_FS) puts(file->path);
		}
		puts("");
	}
	if (types & CONFLICT_PERM) {
		puts("Directories with changed mode or ownership:");
		struct stat st;
		char smode[11];

		list_for_each(_file, &pkg->files) {
			file = _file->data;
			if (file->conflict != CONFLICT_PERM) continue;

			printf("%s %d/%d %s\n",
			       mode_string(file->mode, smode), file->uid,
			       file->gid, file->path);
			if (lstat(file->path, &st) < 0) {
				fprintf(stderr, "can't stat %s%s: %s\n",
				        opt_root, file->path, strerror(errno));
				puts("");
				continue;
			}
			printf("%s %d/%d %s%s\n",
			       mode_string(st.st_mode, smode), st.st_uid,
			       st.st_gid, opt_root, file->path);
			puts("");
		}
	}
	
	return types;
}

static
int adjust_with_fs(pkg_desc_t *pkg) {
	struct stat st;

	list_for_each(_pkg_file, &pkg->files) {
		pkg_file_t *pkg_file = _pkg_file->data;
		if (lstat(pkg_file->path, &st) != 0) {
			if (pkg_file->conflict == CONFLICT_SELF)
				pkg_file->conflict = CONFLICT_NONE;
			continue;
		}
		
		if (S_ISDIR(pkg_file->mode) && S_ISLNK(st.st_mode)) {
			if (stat(pkg_file->path, &st) != 0) {
				fprintf(stderr, "failed to stat %s%s",
					opt_root, pkg_file->path);
				die("");
			}
			if (S_ISDIR(st.st_mode)) {
				dbg("%s%s stored as symlink\n", opt_root,
				    pkg_file->path);
				pkg_file->mode &= ~S_IFDIR;
				pkg_file->mode |= S_IFLNK;
			}
			else pkg_file->conflict = CONFLICT_PERM;
		}
		else if (S_ISDIR(pkg_file->mode) && (
		                        pkg_file->mode != st.st_mode ||
		                        pkg_file->uid != st.st_uid ||
		                        pkg_file->gid != st.st_gid)) {
			pkg_file->conflict = CONFLICT_PERM;
		}
		else if (!S_ISDIR(pkg_file->mode) &&
		                   pkg_file->conflict != CONFLICT_DB &&
		                   pkg_file->conflict != CONFLICT_SELF) {
			pkg_file->conflict = CONFLICT_FS;
		}
	}

	return 0;
}

static
void old_reference(void **ai, void **bj, void *arg) {
	pkg_desc_t *old_pkg = arg;
	pkg_file_t *old_pkgfile = (*(list_entry_t**)ai)->data;

	old_pkgfile->conflict = CONFLICT_REF;

	dbg("ref %s\n", old_pkgfile->path);
	return;
}

static
void self_conflict(void **ai, void **bj, void *arg) {
	pkg_file_t *new_pkgfile = (*(list_entry_t**)ai)->data;
	pkg_file_t *old_pkgfile = (*(list_entry_t**)bj)->data;
	pkg_desc_t *old_pkg = arg;

	new_pkgfile->conflict = CONFLICT_SELF;
	old_pkgfile->conflict = CONFLICT_SELF;

	dbg("self %s\n", new_pkgfile->path);
	return;
}

static
void db_conflict(void **ai, void **bj, void *arg) {
	pkg_file_t *new_pkgfile = (*(list_entry_t**)ai)->data;
	pkg_file_t *db_pkgfile = (*(list_entry_t**)bj)->data;

	// directories can't conflict. sanity tests (e.g. new file is not a
	// dir, but db file is) made while finding fs conflicts, not here.
	if (!S_ISDIR(new_pkgfile->mode)) {
		new_pkgfile->conflict = CONFLICT_DB;
		db_pkgfile->conflict = CONFLICT_DB;
		dbg("db %s\n", new_pkgfile->path);
	}
	return;
}

static
void adjust_with_db(pkg_desc_t *new_pkg, pkg_desc_t *old_pkg) {
	void **dbfiles, **newfiles, **oldfiles;
	size_t dbsize = 0;
	size_t cnt;

	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		if (pkg == old_pkg) continue;
		dbsize += pkg->files.size;
	}
	
	// creating dbfiles sorted array
	dbfiles = fmalloc(dbsize * sizeof(void *));
	cnt = 0;
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		if (pkg == old_pkg) continue;
		list_for_each(_file, &pkg->files) {
			dbfiles[cnt] = _file;
			cnt++;
		}
	}
	qsort(dbfiles, dbsize, sizeof(void *), file_cmp);

	// creating newfiles sorted array
	newfiles = fmalloc(new_pkg->files.size * sizeof(void *));
	cnt = 0;
	list_for_each(_file, &new_pkg->files) {
		newfiles[cnt] = _file;
		cnt++;
	}
	qsort(newfiles, new_pkg->files.size, sizeof(void *), file_cmp);

	// intersections between dbfiles and newfiles are db conflicts
	intersect_uniq(newfiles, new_pkg->files.size, dbfiles, dbsize,
	               file_cmp, db_conflict, NULL, NULL);
	
	if (old_pkg) {
		// creating oldfiles sorted array
		oldfiles = fmalloc(old_pkg->files.size * sizeof(void *));
		cnt = 0;
		list_for_each(_file, &old_pkg->files) {
			oldfiles[cnt] = _file;
			cnt++;
		}
		qsort(oldfiles, old_pkg->files.size, sizeof(void *), file_cmp);
		
		// intersections between newfiles and oldfiles are self
		// conflicts to newfiles. intersected oldfiles are references
		// which should be marked and not removed from filesystem
		// later, as they'll overwritten by newfiles
		intersect_uniq(newfiles, new_pkg->files.size,
		               oldfiles, old_pkg->files.size,
		               file_cmp, self_conflict, NULL, old_pkg);
		
		// intersections between oldfiles and dbfiles are references
		// which must be removed from oldfiles later
		intersect_uniq(oldfiles, old_pkg->files.size,
		               dbfiles, dbsize,
		               file_cmp, old_reference, NULL, old_pkg);
		
		free(oldfiles);
	}
	
	free(newfiles);
	free(dbfiles);
	return;
}

static
int adjust_with_config(const char *path, rule_type_t type) {
	int ret = 1;
	list_for_each(_rule, &config_rules) {
		rule_t *rule = _rule->data;
		if (rule->type != type) continue;
		if (!regexec(&rule->regex, path, 0, NULL, 0)) {
			if (rule->yes) ret = 1;
			else ret = 0;
		}
	}
	return ret;
}

static
void extract_files(struct archive *ar, struct archive_entry *en,
                   void *___file, void *unused) {
	list_entry_t **__file = ___file;
	list_entry_t *_file = *__file;
	pkg_file_t *file = _file->next->data;
	*__file = _file->next;
	
	char path[MAXPATHLEN+1];
	const char *cpath = archive_entry_pathname(en);
	mode_t mode = archive_entry_mode(en);

	if (!adjust_with_config(cpath, INSTALL)) return;

	if (file->conflict != CONFLICT_NONE &&
	                   !adjust_with_config(cpath, UPGRADE)) {
		strcpy(path, PKG_REJECT_DIR);
		strcat(path, cpath);
		archive_entry_set_pathname(en, path);
		if (!S_ISDIR(mode)) {
			fprintf(stderr, "rejecting %s\n", cpath);
			dbg("to %s%s\n", opt_root, path);
		}
		else dbg("rejecting %s to %s%s\n", cpath, opt_root, path);
	}
	else dbg("installing %s%s\n", opt_root, cpath);

	int err;
	if ((err = archive_read_extract(ar, en, ARCHIVE_EXTRACT_OWNER |
	                                        ARCHIVE_EXTRACT_PERM |
	                                        ARCHIVE_EXTRACT_UNLINK)) < 0) {
		const char *strerr = "Operation not permitted";
		err = archive_errno(ar);
		// XXX: investigate why libarchive sets EEXIST where EPERM
		//      expected
		if (err != EEXIST) strerr = archive_error_string(ar);
		fprintf(stderr, "Failed to extract %s%s: %s\n", opt_root,
		        archive_entry_pathname(en), strerr);
	}
	return;
}

static
void list_files(struct archive *ar, struct archive_entry *en, void *_pkg,
                void *unused) {
	pkg_desc_t *pkg = _pkg;
	const char *cpath;
	mode_t mode;
	char *path;
	
	cpath = archive_entry_pathname(en);
	path = fmalloc(strlen(cpath) + 1);
	strcpy(path, cpath);
	
	mode = archive_entry_mode(en);
	if (S_ISDIR(mode)) path[strlen(path)-1] = '\0';

	pkg_file_t *pkg_file = fmalloc(sizeof(pkg_file_t));
	pkg_file->pkg = pkg;
	pkg_file->conflict = CONFLICT_NONE;
	pkg_file->path = path;
	pkg_file->mode = mode;
	pkg_file->uid  = archive_entry_uid(en);
	pkg_file->gid  = archive_entry_gid(en);
	list_append(&pkg->files, pkg_file);

	return;
}

static
void del_old_pkg(pkg_desc_t *old_pkg) {
	list_for_each_r(_file, &old_pkg->files) {
		pkg_file_t *file = _file->data;
		if (!file->conflict) {
			if (remove(file->path)) {
				fprintf(stderr, "Can't remove %s%s: %s\n",
					opt_root, file->path, strerror(errno));
			}
		}
		_file = _file->next;
		list_delete(&old_pkg->files, _file->prev);
		free(file->path);
		free(file);
	}
	list_free(&old_pkg->files);

	list_for_each(_pkg, &pkg_db) {
		if (_pkg->data != old_pkg) continue;
		list_delete(&pkg_db, _pkg);
		break;
	}

	free(old_pkg->name);
	free(old_pkg->version);
	free(old_pkg);
	return;
}

static
void cleanup_pkg(pkg_desc_t *pkg, int remove) {
	list_for_each(_file, &pkg->files) {
		pkg_file_t *file = _file->data;
		if (file->conflict) {
			if (remove) {
				_file = _file->prev;
				list_delete(&pkg->files, _file->next);
				free(file->path);
				free(file);
			}
			else file->conflict = CONFLICT_NONE;
		}
	}
	return;
}

static
void cleanup_pkg_db(pkg_desc_t *except_pkg, int remove) {
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		if (pkg == except_pkg) continue;
		else cleanup_pkg(pkg, remove);
	}
	return;
}

// Returns found conflicts, -1 on other errors, 0 on success
int pkg_add(const char *pkg_path, int opts) {
	FILE *pkgf = NULL, *curdir = NULL;
	pkg_desc_t *pkg = NULL;
	pkg_desc_t *old_pkg;
	int found_conflicts = -1;
	read_config();
	
	pkgf = fopen(pkg_path, "r");
	if (!pkgf) {
		fprintf(stderr, "Can't open package %s: %s\n", pkg_path,
		        strerror(errno));
		goto cleanup;
	}
	curdir = fopen(".", "r");
	if (!curdir) die("Failed to obtain current directory");
	if (chdir(opt_root)) die("Can't chdir to root directory");

	pkg = fmalloc(sizeof(pkg_desc_t));
	list_init(&pkg->files);
	if (pkg_make_desc(pkg_path, pkg)) {
		fprintf(stderr, "'%s' is not a valid package name\n", pkg_path);
		pkg->name = NULL;
		pkg->version = NULL;
		goto cleanup;
	}
	if (do_archive(pkgf, list_files, pkg, NULL)) abort();

	old_pkg = pkg_find_pkg(pkg->name);
	adjust_with_db(pkg, old_pkg);
	adjust_with_fs(pkg);
	found_conflicts = report_conflicts(pkg);
	
	if (found_conflicts & CONFLICT_PERM && !(opts & PKG_ADD_FORCE_PERM)) {
		cleanup_pkg_db(NULL, 0);
		goto cleanup;
	}
	if (found_conflicts & ~CONFLICT_PERM && !(opts & PKG_ADD_FORCE)) {
		cleanup_pkg_db(NULL, 0);
		goto cleanup;
	}

	if (old_pkg) del_old_pkg(old_pkg);
	list_append(&pkg_db, pkg);
	pkg_commit_db();

	list_entry_t *tmp = pkg->files.head;
	do_archive(pkgf, extract_files, &tmp, NULL);

	if (found_conflicts) {
		// In case of forced conflicts, we need clear conflict
		// flags in new package and whole db, and remove from
		// db files which are overwritten by new package.
		cleanup_pkg(pkg, 0);
		cleanup_pkg_db(pkg, 1);
	}

	pkg = NULL;
cleanup:
	if (pkg) {
		list_for_each(_file, &pkg->files) {
			pkg_file_t *file = _file->data;
			_file = _file->prev;
			list_delete(&pkg->files, _file->next);
			free(file->path);
			free(file);
		}
		list_free(&pkg->files);
		free(pkg->name);
		free(pkg->version);
		free(pkg);
	}
	if (pkgf) fclose(pkgf);
	if (curdir) {
		if (fchdir(fileno(curdir)) < 0)
			die("Can't go back to CWD");
		fclose(curdir);
	}
	cleanup_config();
	return found_conflicts;
}
