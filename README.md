# pngspark
### [Sparklines](https://github.com/holman/spark) as PNGs

![histogram](https://cloud.githubusercontent.com/assets/95347/6131872/f78ab708-b11c-11e4-9ae2-fd6cd0ec2b76.png)

## Install
```
make
[sudo] make install [PREFIX=/usr/local]
```

## Usage
```
pngspark [-h height] [-c color] [-s scaling] output.png
```
Pipe numbers to pngspark to add them to the image.

### Options
* `-h height`: the height of the image in pixels
* `-c color`: the color of the foreground, as #rrggbb hex
* `-s scaling`: scaling factor. 0 to do no scaling, 1 to make the minimum
  value be at the bottom of the image

## Examples

Display BTC ticker data
```sh
curl -s https://api.bitcoinaverage.com/history/USD/per_minute_24h_sliding_window.csv | sed 1d | cut -d, -f2 | ./pngspark -o btc.png -s 0.995 -h 80 -c 0066cc
```
![btc](https://cloud.githubusercontent.com/assets/95347/6131571/ed7005c2-b11a-11e4-837c-58b07cd5a9c3.png)

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
