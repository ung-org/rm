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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int force = 0;
static int interactive = 0;
static int recursive = 0;

static int rm(const char *);

static int rm_prompt(const char *p)
{
	char *buf = NULL;
	size_t len = 0;

	if (errno == ENOENT)	// FIXME: this is probably wrong
		return 1;

	fprintf(stderr, "remove %s? ", p);
	getline(&buf, &len, stdin);
	if (!strcasecmp("yes\n", buf) || !(strcasecmp("y\n", buf))) {
		free(buf);
		return 1;
	}
	return 0;
}

int nftw_rm(const char *p, const struct stat *sb, int typeflag, struct FTW *f)
{
	return rm(p);
}

static int rm(const char *p)
{
	struct stat st;
	if (lstat(p, &st) != 0) {
		if (!force) {
			perror(p);
		}
		return 1;
	}

	if (S_ISDIR(st.st_mode)) {
		if (!recursive) {
			fprintf(stderr, "%s: %s\n", p, strerror(EISDIR));
			return 1;
		}

		if (!force && ((access(p, W_OK) != 0 && isatty(fileno(stdin)))
			       || interactive)) {
			if (rm_prompt(p) == 0) {
				return 0;
			}
		}

		if (rmdir(p) != 0) {
			perror(p);
			return 1;
		}
		return 0;
	}

	if (!force && ((access(p, W_OK) != 0 && isatty(fileno(stdin)))
		       || interactive)) {
		if (rm_prompt(p) == 0) {
			return 0;
		}
	}

	if (unlink(p) != 0) {
		perror(p);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, ":fiRr")) != -1) {
		switch (c) {
		case 'f':
			force = 1;
			interactive = 0;
			break;

		case 'i':
			force = 0;
			interactive = 1;
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
		return force ? 0 : 1;
	}

	int ret = 0;
	while (optind < argc) {
		if (recursive) {
			ret |= nftw(argv[optind], nftw_rm, 1024, FTW_DEPTH);
		} else {
			ret |= rm(argv[optind]);
		}
		optind++;
	}

	return ret;
}
