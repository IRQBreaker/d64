CC = gcc
CFLAGS = -Wall -pedantic --std=c99 -O2 -s
PRG = d64

objects = main.o disk.o disasm.o

$(PRG): $(objects)
	$(CC) $(CFLAGS) -o $(PRG) $(objects)

main.o: disk.h disasm.h
disk.o: disk.h
disasm.o: disasm.h

.PHONY: clean mrproper
clean:
	rm -f $(PRG) $(objects)

mrproper:
	git clean -dxf
