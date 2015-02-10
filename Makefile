SRC = $(wildcard *.c)
OBJ = $(SRC:c=o)
BIN = pngspark
PREFIX ?= /usr/local

CFLAGS = -Wall -Wextra -Werror -O2 -std=c99 -pedantic
LDFLAGS = -lz

all: $(BIN)

lupng.o: lupng.c
	$(CC) -c -o $@ $^ -Werror -std=c99 -DLUPNG_USE_ZLIB 

pngspark: pngspark.o main.o lupng.o
	$(CC) -o $@ $^ $(LDFLAGS)

install: $(BIN)
	cp -f $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm $(PREFIX)/bin/$(BIN)

clean:
	rm -f test $(BIN) $(OBJ)

.PHONY: clean install uninstall
