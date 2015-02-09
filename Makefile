SRC = $(wildcard *.c)
OBJ = $(SRC:c=o)
BIN = pngspark
PREFIX ?= /usr/local

CFLAGS = -Wall -Wextra -Werror -O2 -std=c99 -pedantic

all: $(BIN)

pngspark: pngspark.o main.o

install: $(BIN)
	cp -f $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm $(PREFIX)/bin/$(BIN)

clean:
	rm -f test $(BIN) $(OBJ)

.PHONY: clean install uninstall
