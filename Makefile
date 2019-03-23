CC = gcc
CFLAGS = -Wall -pedantic --std=c99 -O2

objects = main.o d64.o

d64: $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects)

main.o: d64.h
d64.o: d64.h

.PHONY: clean
clean:
	rm -f d64 $(objects)
