CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic --std=c99 -O2 -s

prg = d64
objects = main.o disk.o disasm.o basic.o util.o

$(prg): $(objects)
	$(CC) $(CFLAGS) -o $(prg) $(objects)

main.o: disk.h disasm.h basic.h util.h
disk.o: disk.h util.h
disasm.o: disasm.h
basic.o: basic.h util.h
util.o: util.h

.PHONY: clean mrproper cppcheck gtags
clean:
	rm -f $(prg) $(objects)

mrproper:
	git clean -dxf

cppcheck:
	cppcheck --enable=all --language=c --std=c99 --suppress=missingInclude *.c

gtags:
	git ls-files | gtags -f -
