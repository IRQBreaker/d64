#include "t64.h"
#include "util.h"

#include <stdio.h>
#include <ctype.h>
#include <arpa/inet.h>

#define DES_LEN 32
#define USER_DES_LEN 24
#define FNAME_LEN 16
#define MIN_SIZE 86
#define FILE_RECORD_OFFSET 64

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
    "free entry",
    "normal tape file",
    "tape file with header",
    "memory snapshot v0.9, uncompressed",
    "tape block",
    "digitized stream",
    "unknown"
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

    int index = FILE_RECORD_OFFSET;
    file_record *file = (file_record*)&buffer[index];

    printf("Contents:\n");
    for (int i = 1; i <= tape->used; i++) {
        for (int j = 0; j < FNAME_LEN; j++)
            printf("%c", file->fname[j]);
        printf("\n");
        index += sizeof(file_record);
        file = (file_record*)&buffer[index];
    }
}
