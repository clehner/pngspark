#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "lupng.h"
#include "pngspark.h"

static const size_t initial_size = 8;

uint8_t parse_color(const char *color)
{
	return !strcmp("black", color) ? 0x00 : 0x99;
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

int pngspark_draw(struct pngspark *ps, uint8_t *data, size_t width,
		size_t height)
{
	double *values = ps->values;
	uint8_t color = ps->color;
	for (size_t x = 0; x < width; x++) {
		size_t value = values[x] * height;
		data[x] = value;
		for (size_t y = 0; y < height; y++)
			data[x * width + y] = 0x9f;
		/*
		printf("write value: %zu, i: %zu\n", value, 1 * width + x);
		for (size_t y = value+1; y < height; y++)
			data[y * width + x] = color;
			*/
		(void)value;
		(void)color;
		(void)data;
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
    LuImage *img = luImageCreate(ps->num_values, ps->max_value+1, 1, 8);
	if (!img) return 1;
	printf("size: %zu\n", img->dataSize);

	int ret = pngspark_draw(ps, img->data, ps->num_values, ps->max_value);
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

