#include "sid.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

typedef struct
{
    char psid[4];
    uint16_t version;
    uint16_t offset;
    uint16_t laddr;
    uint16_t iaddr;
    uint16_t paddr;
    uint16_t songs;
    uint16_t dsong;
    uint32_t speed;
    char name[32];
    char author[32];
    char copyright[32];
    union
    {
        struct
        {
            uint8_t *data;
        } PACKED sidv1;

        struct
        {
            uint16_t flags;
            uint32_t reserved;
            uint8_t *data;
        } PACKED sidv2;
    } PACKED data;
} PACKED sid_header;

#define MIN_FILE_LENGTH 0x76

void sid(const uint8_t *buffer, const int size)
{
    sid_header *header = (sid_header*)buffer;

    if (strncmp(header->psid, "PSID", 4) != 0 || size < MIN_FILE_LENGTH) {
        fprintf(stderr, "Not a valid SID file\n");
        return;
    }

    printf("Name:            %s\n", header->name);
    printf("Author:          %s\n", header->author);
    printf("Copyright:       %s\n", header->copyright);
    printf("Number of songs: %d\n", ntohs(header->songs));
    printf("Default song:    %d\n", ntohs(header->dsong));
    printf("Speed:           %sHz\n", (ntohl(header->speed) == 0) ? "50" : "60");

    uint8_t *c64;
    if (ntohs(header->version) == 1)
        c64 = (uint8_t *)&header->data.sidv1.data;
    else
        c64 = (uint8_t *)&header->data.sidv2.data;

    uint16_t laddr;
    if (ntohs(header->laddr) == 0)
        laddr = c64[0] + ((c64[1] & 0xff) << 8);
    else
        laddr = ntohs(header->laddr);

    printf("Load address:    0x%04x\n", laddr);
    printf("Init address:    0x%04x\n", ntohs(header->iaddr));
    printf("Play address:    0x%04x\n", ntohs(header->paddr));

}
