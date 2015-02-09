#ifndef __PNGSPARK_H
#define __PNGSPARK_H

struct pngspark {
	int fd;
	int height;
};

int pngspark_init(struct pngspark *, int, int, const char *);
int pngspark_append(struct pngspark *, double);
int pngspark_end(struct pngspark *);

#endif /* __PNGSPARK_H */
