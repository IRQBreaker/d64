#include "disk.h"
#include "disasm.h"
#include "basic.h"
#include "sid.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

typedef enum {BIN, D64, BAS, SID} filetype;

static void printhelp(char *program)
{
    printf("Usage: %s [options] file\n" \
        "  -a address  disassemble from address\n" \
        "  -f          force disassembly\n" \
        "  -i          show illegal opcodes\n" \
        "  -h          this help text\n", basename(program));
}

static filetype get_filetype(const uint8_t *buffer, const char *filename)
{
    // Not enough to determine file type
    if (strlen(filename) < 4)
        return BIN;

    // D64 disk image
    if ((!strncmp(&filename[strlen(filename) - 3], "d64", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "D64", 3)))
        return D64;

    // SID file
    if ((!strncmp(&filename[strlen(filename) - 3], "sid", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "SID", 3)))
        return SID;

    // C64 basic
    if (buffer[0] == 0x01 && buffer[1] == 0x08)
      return BAS;

    return BIN;
}

int main(int argc, char **argv)
{
    int optforce = 0;
    int optillegal = 0;
    uint16_t address = UINT16_MAX;
    char c;
    char *end;
    while ((c = getopt(argc, argv, "fiha:")) != -1) {
        switch (c) {
            case 'a':
                address = strtol(optarg, &end, 0);
                if (end == optarg) {
                    errno = EINVAL;
                    perror("Error");
                    return EXIT_FAILURE;
                }
                break;

            case 'f':
                optforce = 1;
                break;

            case 'i':
                optillegal = 1;
                break;

            case 'h':
            default:
                printhelp(argv[0]);
                return EXIT_SUCCESS;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Missing filename\n");
        printhelp(argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[optind], O_RDONLY);
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

    if (optforce)
        disasm(buffer, st.st_size, address, optillegal);
    else {
        switch (get_filetype(buffer, argv[optind])) {
            case D64:
                showdisk(buffer, st.st_size);
                break;

            case BAS:
                showbasic(buffer, st.st_size);
                break;

            case SID:
                showsid(buffer, st.st_size);
                break;

            case BIN:
            default:
                disasm(buffer, st.st_size, address, optillegal);
                break;
        }
    }

    munmap(buffer, st.st_size);
    close(fd);

    return EXIT_SUCCESS;
}
