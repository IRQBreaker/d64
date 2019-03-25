#include "disk.h"
#include "disasm.h"

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

typedef enum {BIN, D64} filetype;

filetype get_filetype(uint8_t *buffer, char *filename, int size)
{
    (void)buffer; // To be used at a later stage

    int length = strlen(filename);

    if (length < 4)
        return BIN;

    if ((!strncmp(&filename[length - 3], "d64", 3) ||
            !strncmp(&filename[length - 3], "D64", 3)) &&
            validate_disk(size))
        return D64;

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

    switch (get_filetype(buffer, argv[1], st.st_size)) {
        case D64:
            showdisk(buffer);
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
