#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "lupng.h"
#include "pngspark.h"

static const size_t initial_size = 8;

uint32_t parse_color(const char *color_str)
{
	if (!color_str) return 0;
	if (color_str[0] == '#') color_str++;
	uint32_t color = strtol(color_str, NULL, 16);
	return 0xff000000 |
		((color >> 16) | (color & 0x00ff00) | (color & 0xff) << 16);
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

static size_t write_fd(const void *ptr, size_t size, size_t count,
		void *userPtr)
{
	return fwrite(ptr, size, count, (FILE *)userPtr);
}

int pngspark_write(struct pngspark *ps, FILE *file)
{
	size_t height = ps->height;
	size_t width = ps->num_values;
	LuImage *img = luImageCreate(ps->num_values, ps->height, 4, 8);
	if (!img) return 1;

	double *values = ps->values;
	uint32_t *pixels = (uint32_t *)img->data;
	uint32_t color = ps->color;
	double height_scale = (double)height / (double)ps->max_value;

	for (size_t x = 0; x < width; x++) {
		size_t value = height - (values[x] * height_scale);
		for (size_t y = 0; y < value; y++)
			pixels[x + width * y] = 0;
		for (size_t y = value; y < height; y++)
			pixels[x + width * y] = color;
	}

	int ret = luPngWrite(write_fd, file, img);
	luImageRelease(img);
	return ret;
}

int pngspark_end(struct pngspark *ps)
{
	free(ps->values);
	return 0;
}

