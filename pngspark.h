#ifndef __PNGSPARK_H
#define __PNGSPARK_H

#include <stdint.h>

struct pngspark {
	size_t num_values;
	size_t size;
	size_t max_value;
	size_t height;
	uint8_t color;
	double *values;
};

int pngspark_init(struct pngspark *, size_t, const char *);
int pngspark_append(struct pngspark *, double);
int pngspark_write(struct pngspark *, FILE *);
int pngspark_end(struct pngspark *);

#endif /* __PNGSPARK_H */
