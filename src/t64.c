#include "t64.h"
#include "util.h"

#include <stdio.h>
#include <ctype.h>
#include <arpa/inet.h>

#define DES_LEN 32
#define USER_DES_LEN 24
#define FNAME_LEN 16
#define MIN_SIZE 86

typedef struct
{
    char tape_des[DES_LEN];
    uint16_t version;
    uint16_t entries;
    uint16_t used;
    uint16_t fill;
    char user_des[USER_DES_LEN];
} PACKED tape_record;

typedef struct
{
    uint8_t type;
    uint8_t ftype;
    uint16_t start_addr;
    uint16_t end_addr;
    uint16_t fill0;
    uint32_t offset;
    uint32_t fill1;
    char fname[FNAME_LEN];
} PACKED file_record;

const char *types[] = {
    "Free entry",
    "Normal tape file",
    "Tape file with header",
    "Memory snapshot v0.9, uncompressed",
    "Tape block",
    "Digitized stream",
    "Unknown"
};

void t64(const uint8_t *buffer, const int size)
{
    if (size < MIN_SIZE) {
        fprintf(stderr, "Not a valid T64 file.\n");
        return;
    }

    tape_record *tape = (tape_record*)&buffer[0];

    printf("Name: ");
    for (int i = 0; i < USER_DES_LEN; i++)
        printf("%c", (isprint(tape->user_des[i])) ? tape->user_des[i] : ' ');
    printf("\n");

    int type_size = sizeof(types) / sizeof(types[0]);
    int index = sizeof(tape_record);

    printf("Contents:\n");
    for (int i = 0; i < tape->used; i++) {
        file_record *file = (file_record*)&buffer[index];

        for (int j = 0; j < FNAME_LEN; j++)
            printf("%c", (isprint(file->fname[j])) ? file->fname[j] : ' ');

        int cft = file->ftype & ((1 << 3) - 1);
        if (cft > 4)
            cft = c64_file_type_size - 1;

        printf("  %s", c64_file_type[cft]);
        printf("  %s", (file->type >= type_size) ?
                types[type_size - 1] : types[file->type]);

        printf("  0x%04x - 0x%04x", file->start_addr, file->end_addr);
        printf("\n");

        index += sizeof(file_record);
    }
}
