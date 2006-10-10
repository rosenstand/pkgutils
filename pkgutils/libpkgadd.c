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
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>
#include <sys/param.h>
#include <sys/file.h>

#include <pkgutils/pkgutils.h>

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
void cleanup_conflicts(list_t *conflicts) {
	list_for_each(_conflict, conflicts) {
		free(_conflict->data);
	}
}

static
int report_conflicts(list_t *conflicts) {
	int types = CONFLICT_NONE;
	pkg_conflict_t *conflict;

	list_for_each(_conflict, conflicts) {
		conflict = _conflict->data;
		switch (conflict->type) {
			case CONFLICT_DB: 
			case CONFLICT_FS:
			case CONFLICT_PERM:
				types |= conflict->type;
				break;
			default:
				break;
		}
		
	}

	if (types & CONFLICT_DB) {
		puts("Following files conflicting with database:");
		list_for_each(_conflict, conflicts) {
			conflict = _conflict->data;
			if (conflict->type == CONFLICT_DB)
				puts(conflict->file->path);
		}
		puts("");
	}
	if (types & CONFLICT_FS) {
		puts("Following files conflicting with filesystem:");
		list_for_each(_conflict, conflicts) {
			conflict = _conflict->data;
			if (conflict->type == CONFLICT_FS)
				puts(conflict->file->path);
		}
		puts("");
	}
	if (types & CONFLICT_PERM) {
		puts("Following files conflicting by mode or ownership:");
		list_for_each(_conflict, conflicts) {
			conflict = _conflict->data;
			if (conflict->type == CONFLICT_PERM)
				puts(conflict->file->path);
		}
		puts("");
	}

	return types;
}

static
pkg_conflict_t *is_conflicts(list_t *conflicts, const char *path,
                            pkg_conflict_type_t type) {
	list_for_each(_conflict, conflicts) {
		pkg_conflict_t *conflict = _conflict->data;
		if (conflict->type & type &&
                    !strcmp(conflict->file->path, path)) return conflict;
	}
	return NULL;
}

static
int adjust_with_fs(pkg_desc_t *pkg, list_t *conflicts) {
	struct stat st;
	size_t root_size = strlen(opt_root);
	char *tmp;
	size_t path_size = 0;
	pkg_conflict_t *conflict;

	list_for_each(_pkg_file, &pkg->files) {
		pkg_file_t *pkg_file = _pkg_file->data;
		path_size = MAX(path_size, strlen(pkg_file->path));
	}
	tmp = fmalloc(path_size + root_size + 1);
	strcpy(tmp, opt_root);

	list_for_each(_pkg_file, &pkg->files) {
		pkg_file_t *pkg_file = _pkg_file->data;
		tmp[root_size] = '\0';
		strcat(tmp, pkg_file->path);

		if (lstat(tmp, &st) != 0) continue;
		
		pkg_conflict_type_t conflict_type = CONFLICT_FS;
		if (S_ISDIR(pkg_file->mode) && S_ISLNK(st.st_mode)) {
			if (stat(tmp, &st) != 0) {
				printf("can't stat %s", tmp);
				die("");
			}
			if (S_ISDIR(st.st_mode)) {
				dbg("%s stored as LINK\n", tmp);
				pkg_file->mode &= ~S_IFDIR;
				pkg_file->mode |= S_IFLNK;
				conflict = is_conflicts(conflicts,
				                        pkg_file->path,
				                        CONFLICT_DB);
				if (conflict) {
					conflict->type = CONFLICT_LINK;
					continue;
				}
				conflict_type = CONFLICT_LINK;
			}
		}
		else if (S_ISDIR(pkg_file->mode) && (
		                           pkg_file->mode != st.st_mode
		                        || pkg_file->uid != st.st_uid
		                        || pkg_file->gid != st.st_gid)) {
				conflict_type = CONFLICT_PERM;
		}
		else if (S_ISDIR(pkg_file->mode) ||
		         is_conflicts(conflicts, pkg_file->path,
		                      CONFLICT_SELF | CONFLICT_DB)) {
			continue;
		}

		conflict = fmalloc(sizeof(pkg_conflict_t));
		conflict->pkg = pkg;
		conflict->file = pkg_file;
		conflict->type = conflict_type;
		list_append(conflicts, conflict);
	}

	free(tmp);
	return 0;
}

// When upgrading (old_pkg != NULL), then append directories which is
// referenced by other packages. They'll not removed while upgrading.
static
void find_db_refs(pkg_desc_t *old_pkg, list_t *conflicts, pkg_desc_t *db_pkg,
                  pkg_file_t *db_file) {
	if (!old_pkg || db_pkg == old_pkg) return;
	
	list_for_each(_file, &old_pkg->files) {
		pkg_file_t *file = _file->data;
		if (strcmp(db_file->path, file->path)) {
			pkg_conflict_t *conflict;
			conflict = fmalloc(sizeof(pkg_conflict_t));
			conflict->type = CONFLICT_REF;
			conflict->pkg = db_pkg;
			conflict->file = db_file;
			list_append(conflicts,conflict);
			break;
		}
	}
	return;
}

static
void find_db_conflicts(pkg_desc_t *new_pkg, list_t *conflicts,
		pkg_desc_t *db_pkg, pkg_file_t *db_file) {
	pkg_conflict_t *conflict, tmp;
	list_for_each(_new_pkg_file, &new_pkg->files) {
		pkg_file_t *new_pkg_file = _new_pkg_file->data;
		if (!strcmp(new_pkg_file->path, db_file->path)) {
			if (!strcmp(new_pkg->name, db_pkg->name)) {
				tmp.type = CONFLICT_SELF;
				tmp.pkg = new_pkg;
				tmp.file = new_pkg_file;
			}
			else if (!S_ISDIR(db_file->mode)) {
				tmp.type = CONFLICT_DB;
				tmp.pkg = db_pkg;
				tmp.file = db_file;
			}
			else continue;

			conflict = fmalloc(sizeof(pkg_conflict_t));
			*conflict = tmp;
			list_append(conflicts, conflict);
		}
	}
	return;
}

static
void adjust_with_db(pkg_desc_t *new_pkg, pkg_desc_t *old_pkg,
                    list_t *conflicts) {
	list_for_each(_db_pkg, &pkg_db) {
		pkg_desc_t *db_pkg = _db_pkg->data;
		list_for_each(_db_file, &db_pkg->files) {
			pkg_file_t *db_file = _db_file->data;
			find_db_conflicts(new_pkg, conflicts, db_pkg, db_file);
			find_db_refs(old_pkg, conflicts, db_pkg, db_file);
		}
	}
	return;
}

static
int should_reject(const char *path, list_t *conflicts) {
	int reject = 0;
	list_for_each(_conflict, conflicts) {
		pkg_conflict_t *conflict = _conflict->data;
		if (conflict->type != CONFLICT_SELF
		               || strcmp(conflict->file->path, path)) continue;

		list_for_each(_rule, &config_rules) {
			rule_t *rule = _rule->data;
			if (rule->type != UPGRADE) continue;
			if (!regexec(&rule->regex, path, 0, NULL, 0)) {
				if (!rule->yes) reject = 1;
				else reject = 0;
			}
		}
		break;
	}
	return reject;
}

static
int should_install(const char *path) {
	int install = 1;
	list_for_each(_rule, &config_rules) {
		rule_t *rule = _rule->data;
		if (rule->type != INSTALL) continue;
		if (!regexec(&rule->regex, path, 0, NULL, 0)) {
			if (rule->yes) install = 1;
			else install = 0;
		}
	}
	return install;
}

static
void extract_files(struct archive *ar, struct archive_entry *en,
                   void *conflicts, void *unused) {
	const char *cpath;
	char *path;
	cpath = archive_entry_pathname(en);

	if (!should_install(cpath)) return;

	if (should_reject(cpath, conflicts)) {
		path = fmalloc(strlen(opt_root) + sizeof(PKG_REJECT_DIR) +
		               strlen(cpath));
		strcpy(path, opt_root);
		strcat(path, PKG_REJECT_DIR);
		strcat(path, cpath);
		fprintf(stderr, "rejecting %s\n", cpath);
		dbg("to %s\n", path);
	}
	else {
		path = fmalloc(strlen(opt_root) + strlen(cpath) + 1);
		strcpy(path, opt_root);
		strcat(path, cpath);
		dbg("installing %s\n", path);
	}

	archive_entry_set_pathname(en, path);

	int err;
	if ((err = archive_read_extract(ar, en, 0)) < 0) {
		const char *strerr = "Operation not permitted";
		err = archive_errno(ar);
		// XXX: investigate why libarchive sets EEXIST where EPERM
		//      expected
		if (err != EEXIST) strerr = archive_error_string(ar);
		fprintf(stderr, "Failed to extract %s: %s\n", path, strerr);
	}
	free(path);
	return;
}

static
void list_files(struct archive *ar, struct archive_entry *en, void *_files,
                void *unused) {
	list_t *files = _files;
	const char *cpath;
	mode_t mode;
	char *path;
	
	cpath = archive_entry_pathname(en);
	path = fmalloc(strlen(cpath) + 1);
	strcpy(path, cpath);
	
	mode = archive_entry_mode(en);
	if (S_ISDIR(mode)) path[strlen(path)-1] = '\0';
	
	pkg_file_t *pkg_file = fmalloc(sizeof(pkg_file_t));
	pkg_file->path = path;
	pkg_file->mode = mode;
	pkg_file->uid  = archive_entry_uid(en);
	pkg_file->gid  = archive_entry_gid(en);
	list_append(files, pkg_file);

	return;
}

typedef void (*do_archive_fun_t)(struct archive *ar, struct archive_entry *en,
                                 void *arg1, void *arg2);

// Calls func for the every archive entry. Handy function, and also eliminates
// code duplication
int do_archive(const char *pkg_path, do_archive_fun_t func, void *arg1,
               void *arg2) {
	struct archive *ar;
	struct archive_entry *en;
	int err = 0;

	ar = archive_read_new();
	if (!ar) malloc_failed();
	archive_read_support_compression_all(ar);
	archive_read_support_format_all(ar);
	if (archive_read_open_filename(ar, pkg_path, 1024) != ARCHIVE_OK) {
		puts(archive_error_string(ar));
		err = -1;
		goto failed;
	}
	
	while (1) {
		err = archive_read_next_header(ar, &en);
		if (err == ARCHIVE_OK) {
			func(ar, en, arg1, arg2);
		}
		else if (err == ARCHIVE_EOF) {
			err = 0;
			break;
		}
		else {
			puts(archive_error_string(ar));
			err = -1;
			goto failed;
		}
	}

failed:
	archive_read_finish(ar);
	return err;
}

void cleanup_config() {
	list_for_each(_rule, &config_rules) {
		rule_t *rule = _rule->data;
		regfree(&rule->regex);
		free(rule);
	}
	list_free(&config_rules);
	return;
}

// This function transforms string
// from: "abc \"def ghi\""
//   to: "abc\0def ghi\0"
// Returns count of fields fetched (2 in that example).
int proceed_config_line(char *line) {
	size_t i = 0, j = 0, n = 0;
	int esc = 0, quo = 0, skip = 0;
	
	while (1) {
		switch (line[i]) {
			case '\0':
			case '\n':
				if (quo) return -1;
			case '#':
				if (quo) break;
				line[j] = '\0';
				return n;
				break;
			case ' ':
			case '\t':
				if (quo) break;
				skip = 1;

				if (line[i+1] != ' ' && line[i+1] != '\t' &&
				    line[i+1] != '\n' && line[i+1] != '#') {
					n++;
					line[j] = '\0';
					j++;
				}
				break;
			case '\"':
				if (quo) {
					if (!esc) {
						quo = 0;
						skip = 1;
					}
					else esc = 0;
				}
				else {
					quo = 1;
					skip = 1;
				}
				break;
			case '\\':
				if (!quo) return -1;

				if (esc) esc = 0;
				else {
					esc = 1;
					skip = 1;
				}
				break;
			default:
				if (!n) n = 1;
				esc = 0;
				break;
		}

		if (skip) {
			skip = 0;
			i++;
			continue;
		}

		line[j] = line[i];
		j++;
		i++;
	}
	return 0;
}

void read_config() {
	char *config;
	FILE *f;
	char *line = NULL;
	size_t line_size;
	char *pline;
	size_t lineno = 0;
	int tmp;
	rule_t tmprule;
	
	list_init(&config_rules);
	config = fmalloc(strlen(opt_root) + sizeof(PKG_ADD_CONFIG));
	strcpy(config, opt_root);
	strcat(config, PKG_ADD_CONFIG);
	f = fopen(config, "r");
	if (!f) die(config);

	while (1) {
		line = NULL;
		if (getline(&line, &line_size, f) == -1) break;
		lineno++;

		tmp = proceed_config_line(line);
		if (tmp < 0) {
			fprintf(stderr, "%s: parse error at line %d\n", config,
			                                               lineno);
			abort();
		}

		if (tmp == 3 && !strcmp(line, "UPGRADE")) {
			tmprule.type = UPGRADE;
		}
		else if (tmp == 3 && !strcmp(line, "INSTALL")) {
			tmprule.type = INSTALL;
		}
		else if (tmp == 0) {
			free(line);
			continue;
		}
		else {
			fprintf(stderr, "%s: unknown rule: %s\n", config,
			                                          line);
			abort();
		}

		pline = line + strlen(line) + 1;
		if (regcomp(&tmprule.regex, pline, REG_EXTENDED | REG_NOSUB)) {
			fprintf(stderr, "%s: %s: can't compile regex\n",
			        config, pline);
			abort();
		}

		pline = pline + strlen(pline) + 1;
		if (pline[0] == 'Y') tmprule.yes = 1;
		else tmprule.yes = 0;

		rule_t *rule = fmalloc(sizeof(rule_t));
		*rule = tmprule;
		list_append(&config_rules, rule);

		free(line);
	}

	free(config);
	fclose(f);
	return;
}

pkg_desc_t *pkg_find_pkg(const char *name) {
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		if (strcmp(pkg->name, name)) continue;
		return pkg;
	}
	return NULL;
}

static
void pkg_add_pkg(const pkg_desc_t *new_pkg, list_t *conflicts) {
	char *tmp = fmalloc(MAXPATHLEN);
	size_t root_len = strlen(opt_root);
	strcpy(tmp, opt_root);
	// When upgrading, delete old package first. From that point DB
	// conflicts invalidated. SELF conflicting files not removed (they'll
	// overwritten by extract_files anyway). REF psudo conflicts will not
	// removed too.
	list_for_each(_pkg, &pkg_db) {
		pkg_desc_t *pkg = _pkg->data;
		if (strcmp(pkg->name, new_pkg->name)) continue;
		list_for_each_r(_file, &pkg->files) {
			pkg_file_t *file = _file->data;
			if (!is_conflicts(conflicts, file->path,
			                  CONFLICT_SELF | CONFLICT_REF)) {
				tmp[root_len] = '\0';
				strcat(tmp, file->path);
				if (remove(tmp)) {
					fprintf(stderr, "can't delete %s",
					        tmp);
					die("");
				}
			}
			free(file->path);
			free(file);
		}
		list_free(&pkg->files);
		free(pkg->name);
		free(pkg->version);
		_pkg = _pkg->prev;
		list_delete(&pkg_db, _pkg->next);
		free(pkg);
		break;
	}
	free(tmp);

	pkg_desc_t *_pkg = fmalloc(sizeof(pkg_desc_t));
	*_pkg = *new_pkg;
	list_append(&pkg_db, _pkg);
	return;
}

// Returns found conflicts, or -1 on other errors
int pkg_add(const char *pkg_path, int opts) {
	pkg_desc_t pkg;
	pkg_desc_t *old_pkg;
	list_t conflicts;
	int found_conflicts;
	read_config();

	if (pkg_make_desc(pkg_path, &pkg)) {
		fprintf(stderr, "%s does not look like a package.\n",
		        pkg_path);
		return -1;
	}
	
	list_init(&pkg.files);
	if (do_archive(pkg_path, list_files, &pkg.files, NULL)) abort();

	list_init(&conflicts);
	old_pkg = pkg_find_pkg(pkg.name);
	adjust_with_db(&pkg, old_pkg, &conflicts);
	adjust_with_fs(&pkg, &conflicts);
	found_conflicts = report_conflicts(&conflicts);
	if (found_conflicts & CONFLICT_PERM && !(opts & PKG_ADD_FORCE_PERM))
		goto cleanup;
	if (found_conflicts & ~CONFLICT_PERM && !(opts & PKG_ADD_FORCE))
		goto cleanup;

	pkg_add_pkg(&pkg, &conflicts);
	pkg_commit_db();

	do_archive(pkg_path, extract_files, &conflicts, NULL);

cleanup:
	cleanup_conflicts(&conflicts);
	list_free(&conflicts);
	cleanup_config();
	return found_conflicts;
}
