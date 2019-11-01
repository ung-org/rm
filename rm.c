/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <ftw.h>
#include <libgen.h>
#include <limits.h>
#include <langinfo.h>
#include <locale.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef OPEN_MAX
#define OPEN_MAX _POSIX_OPEN_MAX
#endif

#ifndef MAX_CANON
#define MAX_CANON _POSIX_MAX_CANON
#endif

static enum { DEFAULT, INTERACTIVE, FORCE } mode = DEFAULT;
static int recursive = 0;
static int retval = 0;

static int confirm(const char *p)
{
	static regex_t yesre = { 0 };
	static char *yesexpr = NULL;

	if (mode == FORCE) {
		return 1;
	}

	if (!((access(p, W_OK) != 0 && isatty(STDIN_FILENO)) || mode == INTERACTIVE)) {
		return 1;
	}

	fprintf(stderr, "rm: %s? ", p);

	char buf[MAX_CANON];
	fgets(buf, sizeof(buf), stdin);

	if (yesexpr == NULL) {
		yesexpr = nl_langinfo(YESEXPR);
		regcomp(&yesre, yesexpr, REG_EXTENDED | REG_ICASE | REG_NOSUB);
	}

	if (regexec(&yesre, buf, 0, NULL, 0) == 0) {
		return 1;
	}

	return 0;
}

static int rm(const char *p, const struct stat *st, int typeflag, struct FTW *f)
{
	(void)typeflag; (void)f;

	if (st == NULL) {
		fprintf(stderr, "rm: %s: unknown error\n", p);
		retval = 1;

	} else if (S_ISDIR(st->st_mode)) {
		if (!recursive) {
			fprintf(stderr, "rm: %s: %s\n", p, strerror(EISDIR));
			retval = 1;
		} else if (confirm(p) && rmdir(p) != 0) {
			fprintf(stderr, "rm: %s: %s\n", p, strerror(errno));
			retval = 1;
		}

	} else if (confirm(p) && unlink(p) != 0) {
		fprintf(stderr, "rm: %s: %s\n", p, strerror(errno));
		retval = 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	int c;
	while ((c = getopt(argc, argv, "fiRr")) != -1) {
		switch (c) {
		case 'f':
			mode = FORCE;
			break;

		case 'i':
			mode = INTERACTIVE;
			break;

		case 'r':
		case 'R':
			recursive = 1;
			break;

		default:
			return 1;
		}
	}

	if (optind >= argc) {
		if (mode == FORCE) {
			return 0;
		}
		fprintf(stderr, "rm: missing operand\n");
		return 1;
	}

	do {
		char *path = argv[optind];
		char base[strlen(path) + 1];
		strcpy(base, path);
		char *b = basename(base);
		struct stat st;

		if (!strcmp(b, "/") || !strcmp(b, ".") || !strcmp(b, "..")) {
			fprintf(stderr, "rm: %s: %s\n", path, strerror(EINVAL));
			retval = 1;

		} else if (lstat(path, &st) != 0) {
			if (mode == FORCE) {
				continue;
			}
			fprintf(stderr, "rm: %s: %s\n", path, strerror(errno));
			retval = 1;

		} else if (recursive) {
			nftw(path, rm, OPEN_MAX, FTW_DEPTH | FTW_PHYS);

		} else {
			rm(path, &st, 0, NULL);
		}

	} while (++optind < argc);

	return retval;
}
