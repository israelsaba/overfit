CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
PREFIX ?= $(HOME)/.local

all: overfit

overfit: src/overfit.c
	$(CC) $(CFLAGS) -o $@ $<

install: overfit
	mkdir -p "$(PREFIX)/bin"

	cp overfit "$(PREFIX)/bin/overfit"

clean:
	rm -f overfit

.PHONY: all install clean
