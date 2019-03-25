CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic --std=c99 -O2 -s
PRG = d64

objects = main.o disk.o disasm.o basic.o util.o

$(PRG): $(objects)
	$(CC) $(CFLAGS) -o $(PRG) $(objects)

main.o: disk.h disasm.h basic.h util.h
disk.o: disk.h
disasm.o: disasm.h
basic.o: basic.h util.h
util.o: util.h

.PHONY: clean mrproper
clean:
	rm -f $(PRG) $(objects)

mrproper:
	git clean -dxf
