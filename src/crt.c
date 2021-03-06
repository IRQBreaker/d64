#include "crt.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

typedef struct
{
    char signature[16];
    uint32_t fhlen;
    uint16_t version;
    uint16_t hwtype;
    uint8_t exrom_line;
    uint8_t game_line;
    uint8_t reserved[6];
    char name[32];
} PACKED cartridge;

typedef struct
{
    char signature[4];
    uint32_t plen;
    uint16_t ctype;
    uint16_t bank;
    uint16_t loadaddr;
    uint16_t size;
    uint8_t *data;
} PACKED chip;

const char *type[] = {
    "Normal cartridge",
    "Action Replay",
    "KSC Power Cartridge",
    "Final Cartridge III",
    "Simons Basic",
    "Ocean",
    "Expert Cartridge",
    "Fun Play, Power Play",
    "Super Games",
    "Atomic Power",
    "Epyx Fastload",
    "Westermann Learning",
    "Rex Utility",
    "Final Cartridge I",
    "Magic Formel",
    "C64 Game System, System 3",
    "Warpspeed",
    "Dinamic",
    "Zaxxon, Super Zaxxon (SEGA)",
    "Magic Desk, Domark, HES Australia",
    "Super Snapshot 5",
    "Comal-80",
    "Structured Basic",
    "Ross",
    "Dela EP64",
    "Dela EP7x8",
    "Dela EP256",
    "Rex EP256",
    "Mikro Assembler",
    "Final Cartridge Plus",
    "Action Replay 4",
    "Stardos",
    "EasyFlash",
    "EasyFlash Xbank",
    "Capture",
    "Action Replay 3",
    "Retro Replay",
    "MMC64",
    "MMC Replay",
    "IDE64",
    "Super Snapshot V4",
    "IEEE-488",
    "Game Killer",
    "Prophet64",
    "EXOS",
    "Freeze Frame",
    "Freeze Machine",
    "Snapshot64",
    "Super Explode V5.0",
    "Magic Voice",
    "Action Replay 2",
    "MACH 5",
    "Diashow-Maker",
    "Pagefox",
    "Kingsoft",
    "Silverrock 128K Cartridge",
    "Formel 64",
    "RGCD",
    "RR-Net MK3",
    "EasyCalc",
    "GMod2",
    "Unknown"
};

void crt(const uint8_t *buffer, const int size)
{
    if (size < (int)(sizeof(cartridge) + sizeof(chip))) {
        fprintf(stderr, "Not a valid cartridge image.\n");
        return;
    }

    cartridge *crt = (cartridge*)&buffer[0];
    chip *ch = (chip*)&buffer[ntohl(crt->fhlen)];

    if (strncmp(crt->signature, "C64 CARTRIDGE", 13) != 0 ||
            strncmp(ch->signature, "CHIP", 4) != 0) {
        fprintf(stderr, "Not a valid cartridge image.\n");
        return;
    }

    printf("Name: %s\n", crt->name);

    int tsize = sizeof(type) / sizeof(type[0]);
    printf("Type: %s\n", (ntohs(crt->hwtype) > tsize) ?
            type[tsize - 1] : type[ntohs(crt->hwtype)]);

    printf("Total packet length: %d\n", ntohl(ch->plen));
    printf("Chip type: ");
    switch (ch->ctype) {
        case 2:
            printf("Flash ROM");
            break;
        case 1:
            printf("RAM, no ROM data");
            break;
        case 0:
        default:
            printf("ROM");
            break;
    }
    printf("\nBank number: %d\n", ntohs(ch->bank));
    printf("Load address: %d\n", ntohs(ch->loadaddr));
    printf("ROM image size: %d\n", ntohs(ch->size));
}
