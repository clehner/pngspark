# pngspark
### [Sparklines](https://github.com/holman/spark) as PNGs

## Install
```
make
[sudo] make install [PREFIX=/usr/local]
```

## Usage
```
pngspark [-h height] [-c color] [-s scaling] output.png
```

### Options
* `-h height`: the height of the image in pixels
* `-c color`: the color of the foreground, as #rrggbb hex
* `-s scaling`: scaling factor. 0 to do no scaling, 1 to make the minimum
  value be at the bottom of the image

## API

```c
struct pngspark;
```
A png spark instance: collection of values and settings.

```c
int pngspark_init(struct pngspark *ps, size_t height, const char *color, double scaling);
```
Initialize a pngspark struct.

```c
int pngspark_append(struct pngspark *ps, double, value);
```
Append a value to the pngspark.

```c
int pngspark_write(struct pngspark *ps, FILE *file);
```
Process the values and write as a PNG to a file.

```c
int pngspark_end(struct pngspark *ps);
```
Release allocated memory for the pngspark. (Does not free `ps`, however).

## License

MIT
