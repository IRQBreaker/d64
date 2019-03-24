CC = gcc
CFLAGS = -Wall -pedantic --std=c99 -O2 -s

objects = main.o disk.o disasm.o

d64: $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects)

main.o: disk.h disasm.h
d64.o: disk.h
disasm.o: disasm.h

.PHONY: clean mrproper
clean:
	rm -f d64 $(objects)

mrproper:
	git clean -dxf
