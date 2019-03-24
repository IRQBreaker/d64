#include "disk.h"
#include "disasm.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

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

    if (!validate_disk(st.st_size)) {
        fprintf(stderr, "Not a valid D64 file\n");
        close(fd);
        return EXIT_FAILURE;
    }

    char *buffer = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error");
        close(fd);
        return EXIT_FAILURE;
    }

    showdisk(buffer);
    disasm(buffer, st.st_size);

    munmap(buffer, st.st_size);
    close(fd);

    return EXIT_SUCCESS;
}
