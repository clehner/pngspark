#include "pngspark.h"

int pngspark_init(struct pngspark *ps, int fd, int height, const char *color)
{
	ps->fd = fd;
	ps->height = height;
	(void)color;
	return 0;
}

int pngspark_append(struct pngspark *ps, double value)
{
	(void)ps;
	(void)value;
	return 0;
}

int pngspark_end(struct pngspark *ps)
{
	(void)ps;
	return 0;
}

