/*
 * UNG's Not GNU
 * 
 * Copyright (c) 2011, Jakob Kaivo <jakob@kaivo.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ftw.h>

const char *rm_desc = "remove directory entries";
const char *rm_inv = "rm [-fiRr] file...";

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
		if (!force)
			perror(p);
		return 1;
	}

	if (S_ISDIR(st.st_mode)) {
		if (!recursive) {
			fprintf(stderr, "%s: %s\n", p, strerror(EISDIR));
			return 1;
		}
		if (!force && ((access(p, W_OK) != 0 && isatty(fileno(stdin)))
			       || interactive)) {
			if (rm_prompt(p) == 0)
				return 0;
		}
		if (rmdir(p) != 0)
			perror(p);
		return 0;
	}

	if (!force && ((access(p, W_OK) != 0 && isatty(fileno(stdin)))
		       || interactive)) {
		if (rm_prompt(p) == 0)
			return 0;
	}

	if (unlink(p) != 0)
		perror(p);
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

	if (optind >= argc)
		return 1;

	while (optind < argc) {
		if (recursive)
			nftw(argv[optind], nftw_rm, 1024, FTW_DEPTH);
		else
			rm(argv[optind]);
		optind++;
	}

	return 0;
}
