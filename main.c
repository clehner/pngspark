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

int main(int argc, char *argv[])
{
	if (isatty(fileno(stdin)) && (argc < 2 ||
				!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		return 1;
	}

	struct pngspark ps;
	const char *color = "#000000";
	const char *filename = "pngspark.png";
	int height = 10;

	for (int i = 1; i < argc; i++) {
		char c = argv[i][1];
		if(argv[i][0] != '-' || argv[i][2])
			c = -1;
		switch(c) {
			case 'c':
				if (++i < argc) color = argv[i];
				break;
			case 'f':
				if (++i < argc) filename = argv[i];
				break;
			case 'h':
				if (++i < argc) height = atoi(argv[i]);
				break;
			default:
				fprintf(stderr, "Usage: %s [-h|--help] VALUE,...\n", argv[0]);
		}
	}

	int fd = open(filename, O_CREAT | O_RDWR, 0644);
	if (!fd) err(1, "unable to open file %s", filename);

	if (pngspark_init(&ps, fd, height, color) < 0)
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

	if (pngspark_end(&ps) < 0)
		return 1;

	return 0;
}
