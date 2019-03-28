#include "disk.h"
#include "disasm.h"
#include "basic.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

typedef enum {BIN, D64, BAS} filetype;

static filetype get_filetype(const uint8_t *buffer, const char *filename)
{
    // Not enough to determine file type
    if (strlen(filename) < 4)
        return BIN;

    // D64 disk image
    if ((!strncmp(&filename[strlen(filename) - 3], "d64", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "D64", 3)))
        return D64;

    // C64 basic
    if (buffer[0] == 0x01 && buffer[1] == 0x08)
      return BAS;

    return BIN;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Error");
        close(fd);
        return EXIT_FAILURE;
    }

    uint8_t *buffer = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error");
        close(fd);
        return EXIT_FAILURE;
    }

    switch (get_filetype(buffer, argv[1])) {
        case D64:
            showdisk(buffer, st.st_size);
            break;

        case BAS:
            showbasic(buffer, st.st_size);
            break;

        case BIN:
        default:
            disasm(buffer, st.st_size, 0);
            break;
    }

    munmap(buffer, st.st_size);
    close(fd);

    return EXIT_SUCCESS;
}
