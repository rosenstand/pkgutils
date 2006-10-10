/*
  pkgdiet - tool for cutting off parts of packages
  Copyright (C) 2006  Anton Vorontsov <cbou@users.sf.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _GNU_SOURCE // using GNU version of getopt_long(3)
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <getopt.h>
#include <archive.h>
#include <archive_entry.h>

#define PKG_EXT       ".pkg.tar.gz"
#define DIET_POSTFIX  "#diet"
#define DEF_CONFIG    PREFIX"/etc/pkgdiet.conf"
#define BUF_SIZE      8192

#define CONFIG_MAXLINE  512

struct config_rule {
	regex_t regex;
	int yes;
};

char *config = DEF_CONFIG;
char *outdir = NULL;
int use_config_outdir = 0;
int verbose = 1;
struct config_rule *rules;

char *base_filename(char *name) {
	char *slash = strrchr(name, '/');
	if (slash) name = slash+1;
	return name;
}

int ar_die(struct archive *ar) {
	puts(archive_error_string(ar));
	exit(archive_errno(ar));
	return 1;
}

int die(const char *str) {
	printf("%s: ", str);
	puts(strerror(errno));
	exit(1);
	return 1;
}

void print_usage(const char *argv0, int ret) {
	puts("pkgdiet "VERSION" (tool for cutting off parts of packages)\n");
	puts("Copyright (C) 2006  Anton Vorontsov\n"
	     "This is free software.  You may redistribute copies of it \n"
	     "under the terms of the GNU General Public License \n"
	     "<http://www.gnu.org/licenses/gpl.html>.\n"
	     "There is NO WARRANTY, to the extent permitted by law.\n");

	printf("Usage: %s [OPTIONS] FILES\n\n", argv0);

	puts("	-c --config=file    specify alternate config file");
	puts("	-o --outdir=dir     directory where diet files will be saved");
	puts("	-s --silent         suppress notices");
	puts("	-h --help           show this help");
	exit(ret);
	return;
}

void parse_args(int argc, char *argv[]) {
	const struct option lopts[] = {
		{"config", 1, NULL, 'c'},
		{"outdir", 1, NULL, 'o'},
		{"silent", 0, NULL, 's'},
		{"help",   0, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	const char opts[] = "c:o:sh";
	int opt;

	while (1) {
		opt = getopt_long(argc, argv, opts, lopts, NULL);
		if (opt == -1) break;
		switch (opt) {
			case 'c':
				config = optarg;
				break;
			case 'o':
				outdir = optarg;
				break;
			case 's':
				verbose = 0;
				break;
			case 'h':
				print_usage(argv[0], 0);
				break;
			case '?':
				print_usage(argv[0], 1);
				exit(1);
				break;
			default:
				break;
		}
	}
	
	if (optind >= argc) print_usage(argv[0], 1);

	return;
}


// This function transforms string
// from: "abc \"def ghi\""
//   to: "abc\0def ghi\0"
// Returns count of fields fetched (2 in that example).
int config_proceed_line(char *line) {
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

void config_read() {
	FILE *f;
	char line[CONFIG_MAXLINE];
	char *pline;
	size_t rules_lim = 50;
	size_t n = 0;
	size_t lineno = 0;
	int tmp;
	
	rules = malloc(sizeof(struct config_rule)*rules_lim);
	rules || die("malloc failed");

	f = fopen(config, "r");
	if (!f) die(config);

	while (1) {
		if (!fgets(line, CONFIG_MAXLINE, f)) break;
		lineno++;
		tmp = config_proceed_line(line);
		if (tmp < 0) {
			fprintf(stderr, "%s: parse error at line %d\n", config,
			                                               lineno);
			exit(1);
		}
		
		pline = line;
		if (tmp == 2 && !strcmp(line, "OUTDIR")) {
			pline = pline + strlen(pline) + 1;
			#ifdef DEBUG
			printf("config outdir: %s\n", pline);
			#endif
			if (!outdir) {
				outdir = malloc(CONFIG_MAXLINE);
				outdir || die("malloc failed");
				strcpy(outdir, pline);
				use_config_outdir = 1;
			}
			continue;
		}
		else if (tmp != 3 || strcmp(line, "INSTALL")) continue;

		pline = pline + strlen(pline) + 1;

		if (regcomp(&rules[n].regex, pline, REG_EXTENDED |
		                                    REG_NOSUB)) {
			printf("%s: can't compile regex\n", pline);
			exit(1);
		}
	
		pline = pline + strlen(pline) + 1;
		if (pline[0] == 'Y') rules[n].yes = 1;
		else rules[n].yes = 0;
		
		#ifdef DEBUG
		printf("config: %s %s\n", line + strlen(line) + 1, pline);
		#endif

		n++;
		if (n >= rules_lim) {
			rules_lim += 50;
			rules = realloc(rules, rules_lim);
			rules || die("realloc failed");
		}

		rules[n].yes = -1;
	}

	rules[n].yes = -1;

	fclose(f);
	return;
}

void config_cleanup() {
	size_t i = 0;
	while (1) {
		if (rules[i].yes < 0) break;
		regfree(&rules[i].regex);
		i++;
	}
	free(rules);
	if (use_config_outdir) free(outdir);
	return;
}

int should_archive(const char *pathname) {
	size_t i = 0;
	int should = 1;

	while (1) {
		if (rules[i].yes < 0) break;
		if (!regexec(&rules[i].regex, pathname, 0, NULL, 0)) {
			if (rules[i].yes) should = 1;
			else should = 0;
		}
		i++;
	}

	return should;
}

void proceed_file(struct archive *iar,struct archive *oar, 
                  struct archive_entry *ien) {
	struct archive_entry *oen;
	mode_t m;
	const char *pathname;
	char buf[BUF_SIZE];
	int ret;
	
	pathname = archive_entry_pathname(ien);
	if (!pathname) ar_die(iar);
	if (!should_archive(pathname)) return;

	oen = archive_entry_new();

	m = archive_entry_mode(ien);
	archive_entry_copy_stat(oen, archive_entry_stat(ien));
	archive_entry_set_pathname(oen, pathname);
	archive_entry_set_hardlink(oen, archive_entry_hardlink(ien));
	archive_entry_set_symlink(oen, archive_entry_symlink(ien));
	archive_write_header(oar, oen) && ar_die(oar);

	while (S_ISREG(m)) {
		ret = archive_read_data(iar, buf, BUF_SIZE);
		if (ret <= 0) break;
		ret = archive_write_data(oar, buf, ret);
		if (ret == -1) ar_die(oar);
	}

	archive_entry_free(oen);
	return;
}

void proceed_archive(const char *ifilename, const char *ofilename) {
	struct archive *iar, *oar;
	struct archive_entry *ien;
	int ret;

	// input archive
	iar = archive_read_new();
	if (!iar) ar_die(iar);
	archive_read_support_compression_all(iar);
	archive_read_support_format_all(iar);
	archive_read_open_file(iar, ifilename, 10240) && ar_die(iar);

	// output archive
	oar = archive_write_new();
	if (!oar) ar_die(oar);
	archive_write_set_compression_gzip(oar);
	archive_write_set_format_pax_restricted(oar);
	archive_write_open_file(oar, ofilename) && ar_die(oar);

	// proceed files
	while (1) {
		ret = archive_read_next_header(iar, &ien);
		if (ret == ARCHIVE_OK) proceed_file(iar, oar, ien);
		else if (ret == ARCHIVE_EOF) break;
		else ar_die(iar);
	}

	// cleanup
	archive_write_close(oar) && ar_die(oar);
	archive_write_finish(oar);
	archive_read_close(iar) && ar_die(oar);
	archive_read_finish(iar);
	return;
}

char *make_ofilename(const char *ifilename) {
	char *ofilename;
	size_t len;

	len = strlen(ifilename);
	
	if (len < sizeof(PKG_EXT) || strcmp(ifilename + len - (sizeof(PKG_EXT)
	                                                      - 1), PKG_EXT)) {
		verbose && fprintf(stderr, "%s: not a package\n", ifilename);
		return NULL;
	}
	else if (len >= sizeof(DIET_POSTFIX PKG_EXT) && !strcmp(ifilename + len
	         - (sizeof(DIET_POSTFIX PKG_EXT) - 1), DIET_POSTFIX PKG_EXT)) {
		#ifdef DEBUG
		fprintf(stderr, "%s: already on diet\n", ifilename);
		#endif
		return NULL;
	}
	
	ofilename = malloc(len + sizeof("#diet"));
	ofilename || die("malloc failed");
	strncpy(ofilename, ifilename, len - (sizeof(PKG_EXT) - 1));
	strcpy(ofilename+len-(sizeof(PKG_EXT) -1), DIET_POSTFIX PKG_EXT);
	
	if (outdir) {
		char *obasename;
		obasename = base_filename(ofilename);
		obasename = strdup(obasename);
		free(ofilename);
		ofilename = malloc(strlen(obasename) + strlen(outdir) + 2);
		ofilename || die("malloc failed");
		sprintf(ofilename, "%s/%s", outdir, obasename);
		free(obasename);
	}
	
	#ifdef DEBUG
	printf("ifilename: %s\n", ifilename);
	printf("ofilename: %s\n", ofilename);
	#endif
	return ofilename;
}

int main(int argc, char *argv[]) {
	char *ifilename, *ofilename;
	
	parse_args(argc, argv);
	config_read();

	while (optind < argc) {
		ifilename = argv[optind];
		optind++;
		ofilename = make_ofilename(ifilename);
		if (!ofilename) continue;
		proceed_archive(ifilename, ofilename);
		printf("Successfully wrote %s\n", ofilename);
		free(ofilename);
	}

	config_cleanup();
	exit(0);
	return 0;
}
