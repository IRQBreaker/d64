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

void showsid(const uint8_t *buffer, const int size)
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
}
