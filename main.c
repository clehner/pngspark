#define _POSIX_SOURCE
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pngspark.h"

char float_chars[(unsigned char)-1] = {
	['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1,
	['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1,
	['.'] = 1, ['-'] = 1, ['e'] = 1, ['E'] = 1, 0
};

void usage(const char *progname)
{
	errx(1, "Usage: %s output.png [-h height] "
			"[-c color] [-s scaling]", progname);
}

int main(int argc, char *argv[])
{
	struct pngspark ps;
	const char *color = "#000000";
	const char *filename = NULL;
	double scaling = 0.8;
	int height = 80;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (filename) {
				usage(argv[0]);
			} else {
				filename = argv[i];
			}
		} else if (argv[i][2]) {
			errx(1, "extra argument: %s", argv[i]);
		} else switch (argv[i][1]) {
			case 'c':
				if (++i < argc) color = argv[i];
				break;
			case 'h':
				if (++i < argc) height = atoi(argv[i]);
				break;
			case 's':
				if (++i < argc) scaling = atof(argv[i]);
				break;
		}
	}

	if (!filename) {
		usage(argv[0]);
	}

	FILE *file = fopen(filename, "w");
	if (!file) err(1, "unable to open file %s", filename);

	if (pngspark_init(&ps, height, color, scaling) < 0)
		return 1;

	char c;
	char buffer[32];
	do {
		unsigned int i = 0;
		for (c = getchar(); float_chars[(unsigned char)c]; c = getchar())
			if (i >= sizeof buffer) {
				warnx("number too big: %.*s\n", (int)(sizeof buffer), buffer);
				break;
			} else
				buffer[i++] = c;
		if (!i) continue;
		buffer[i] = '\0';
		double value = atof(buffer);
		if (pngspark_append(&ps, value) < 0)
			return 1;
	} while (c != EOF);

	if (pngspark_write(&ps, file) < 0)
		return 1;

	if (pngspark_end(&ps) < 0)
		return 1;

	if (fclose(file) < 0)
		err(1, "close %s", filename);

	return 0;
}
