#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "lupng.h"
#include "pngspark.h"

static const size_t initial_size = 8;

uint32_t parse_color(const char *color)
{
	return !strcmp("black", color) ? 0xff000000 : 0xff999989;
}

int pngspark_init(struct pngspark *ps, size_t height, const char *color)
{
	ps->size = initial_size;
	ps->num_values = 0;
	ps->values = malloc(initial_size * sizeof(double));
	if (!ps->values) return 1;
	ps->color = parse_color(color);
	ps->height = height;
	ps->max_value = 0;
	return 0;
}

int pngspark_append(struct pngspark *ps, double value)
{
	size_t i = ps->num_values++;
	if (i >= ps->size) {
		ps->values = realloc(ps->values, sizeof(double) * (ps->size <<= 1));
		if (!ps->values) return 1;
	}
	ps->values[i] = value;
	if (value > ps->max_value)
		ps->max_value = value;
	return 0;
}

int pngspark_draw(struct pngspark *ps, uint32_t *data, size_t width,
		size_t height)
{
	double *values = ps->values;
	uint32_t color = ps->color;
	double height_scale = (double)height / (double)ps->max_value;
	for (size_t x = 0; x < width; x++) {
		size_t value = values[x] * height_scale;
		for (size_t y = 0; y < value; y++)
			data[x + width * y] = 0;
		for (size_t y = value; y <= height; y++)
			data[x + width * y] = color;
	}
	return 0;
}

static size_t write_fd(const void *ptr, size_t size, size_t count,
		void *userPtr)
{
    return fwrite(ptr, size, count, (FILE *)userPtr);
}

int pngspark_write(struct pngspark *ps, FILE *file)
{
    LuImage *img = luImageCreate(ps->num_values, ps->max_value+1, 4, 8);
	if (!img) return 1;
	printf("size: %zu\n", img->dataSize);

	int ret = pngspark_draw(ps, (uint32_t *)img->data, ps->num_values,
			ps->max_value);
	if (!ret)
		ret = luPngWrite(write_fd, file, img);
	luImageRelease(img);
	return ret;
}

int pngspark_end(struct pngspark *ps)
{
	free(ps->values);
	return 0;
}

