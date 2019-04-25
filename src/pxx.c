#include "pxx.h"
#include "basic.h"
#include "util.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define SIG_LEN 7
#define FNAME_LEN 15

typedef struct
{
    char signature[SIG_LEN];
    uint8_t fill0;
    uint8_t filename[FNAME_LEN];
    uint8_t fill1;
    uint8_t rel_size;
    uint8_t start;
} PACKED pheader;

void pxx(const uint8_t *buffer, const int size)
{
    pheader *p = (pheader*)&buffer[0];

    if (size < (int)sizeof(pheader) ||
        strncmp(p->signature, "C64File", SIG_LEN) != 0) {
        fprintf(stderr, "Not a valid Pxx file\n");
        return;
    }

    printf("Contents:\n");
    for (int i = 0; i < FNAME_LEN; i++)
        printf("%c", isprint(pet_asc[p->filename[i]]) ?
            pet_asc[p->filename[i]] : ' ');

    uint8_t *data = (uint8_t *)&buffer[sizeof(pheader)];
    uint16_t startaddr = data[0] + ((data[1] & 0xff)<< 8);

    printf("   $%04x - $%04lx\n", startaddr,
        (size - sizeof(pheader)) - startaddr);

    if (p->rel_size == 0) {
        printf("\nListing:\n");
        basic(data, size - sizeof(pheader));
    }
}
