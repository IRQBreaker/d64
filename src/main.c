#include "disk.h"
#include "disasm.h"
#include "basic.h"
#include "sid.h"
#include "crt.h"
#include "t64.h"
#include "pxx.h"

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

typedef enum {BIN, D64, BAS, SID, CRT, T64, PXX} filetype;

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

    if ((!strncmp(&filename[strlen(filename) - 3], "d64", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "D64", 3)))
        return D64;

    if ((!strncmp(&filename[strlen(filename) - 3], "sid", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "SID", 3)))
        return SID;

    if ((!strncmp(&filename[strlen(filename) - 3], "crt", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "CRT", 3)))
        return CRT;

    if ((!strncmp(&filename[strlen(filename) - 3], "t64", 3) ||
            !strncmp(&filename[strlen(filename) - 3], "T64", 3)))
        return T64;

    if ((!strncmp(&filename[strlen(filename) - 3], "p", 1) ||
          !strncmp(&filename[strlen(filename) - 3], "P", 1))) {
        int num = atoi(&filename[strlen(filename) - 2]);
        if (num > 0 || num < 100)
            return PXX;
    }

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
                disk(buffer, st.st_size);
                break;

            case BAS:
                basic(buffer, st.st_size);
                break;

            case SID:
                sid(buffer, st.st_size);
                break;

            case CRT:
                crt(buffer, st.st_size);
                break;

            case T64:
                t64(buffer, st.st_size);
                break;

            case PXX:
                pxx(buffer, st.st_size);
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
