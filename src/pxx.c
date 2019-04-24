#include "pxx.h"
#include "basic.h"
#include "util.h"

#include <stdio.h>
#include <ctype.h>

// Bytes   Description
// $00-$06   ASCII string "C64File"
// $07   Always $00
// $08-$17   Filename in PETASCII, padded with $00 (not $A0, like a D64)
// $18   Always $00
// $19   REL file record size ($00 if not a REL file)
// $1A-??  Program data

#define SIG_LEN 7
#define FNAME_LEN 15

typedef struct
{
    char signature[SIG_LEN];
    uint8_t fill0;
    uint8_t filename[FNAME_LEN];
    uint8_t fill1;
    uint8_t rel_size;
    uint8_t *data;
} PACKED pheader;

void pxx(const uint8_t *buffer, const int size)
{
    if (size < (int)sizeof(pheader)) {
        fprintf(stderr, "Not a valid Pxx file\n");
        return;
    }

    pheader *p = (pheader*)&buffer[0];

    for (int i = 0; i < FNAME_LEN; i++)
        printf("%c", isprint(pet_asc[p->filename[i]]) ?
            pet_asc[p->filename[i]] : ' ');
    printf("\n");
}
