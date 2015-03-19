#ifndef __PNGSPARK_H
#define __PNGSPARK_H

#include <stdint.h>

struct pngspark {
	size_t num_values;
	size_t size;
	size_t height;
	uint32_t color;
	double *values;
	double max_value;
	double min_value;
	double scaling;
};

int pngspark_init(struct pngspark *, size_t, const char *, double);
int pngspark_append(struct pngspark *, double);
int pngspark_write(struct pngspark *, const char *);
void pngspark_end(struct pngspark *);

#endif /* __PNGSPARK_H */
