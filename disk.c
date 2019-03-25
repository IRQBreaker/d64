#include "disk.h"

#include <stdio.h>
#include <ctype.h>

#define SIZE_35_TRACK             174848
#define SIZE_35_TRACK_ERROR       175531
#define SIZE_40_TRACK             196608
#define SIZE_40_TRACK_ERROR       197376
#define NO_OF_DIRENTRY_PER_SECTOR 8
#define BAM_NO_OF_ENTRIES         35
#define BAM_BITMAP_SIZE           3
#define BAM_DISKNAME_LENGTH       16
#define BAM_DISKID_LENGTH         2
#define BAM_DOSTYPE_LENGTH        2
#define BAM_A0_FILL_1_LENGTH      2
#define BAM_A0_FILL_3_LENGTH      4
#define BAM_DISKINFO_LENGTH       1 + BAM_A0_FILL_1_LENGTH + BAM_DISKID_LENGTH + BAM_DOSTYPE_LENGTH + BAM_A0_FILL_3_LENGTH
#define BAM_EXTENDED_INFO_LENGTH  85
#define FILETYPE_SAVE_BIT         5
#define FILETYPE_LOCKED_BIT       6
#define FILETYPE_CLOSED_BIT       7
#define FILETYPE_DEL              0
#define FILETYPE_SEQ              1
#define FILETYPE_PRG              2
#define FILETYPE_USR              3
#define FILETYPE_REL              4
#define FILENAME_LENGTH           16
#define GEOS_INFO_LENGTH          6

#define PACKED __attribute__ ((__packed__))

typedef struct
{
    uint8_t track;
    uint8_t sector;
} PACKED track_sector;

typedef struct
{
    uint8_t free_sectors;
    uint8_t bitmap[BAM_BITMAP_SIZE];
} PACKED bam_entry;

typedef struct
{
    track_sector first_entry;
    uint8_t dos_version;
    uint8_t dummy1;
    bam_entry bam_entries[BAM_NO_OF_ENTRIES];
    uint8_t diskname[BAM_DISKNAME_LENGTH];
    uint8_t diskinfo[BAM_DISKINFO_LENGTH];
    uint8_t extinfo[BAM_EXTENDED_INFO_LENGTH];
} PACKED bam_block;

typedef struct
{
    track_sector next_dir_entry;
    uint8_t filetype;
    uint8_t filetrack;
    uint8_t filesector;
    uint8_t filename[FILENAME_LENGTH];
    track_sector rel_file;
    uint8_t rel_lenght;
    uint8_t geosinfo[GEOS_INFO_LENGTH];
    uint8_t fs_sizel;
    uint8_t fs_sizeh;
} PACKED dir_entry;

typedef struct
{
    dir_entry dentry[NO_OF_DIRENTRY_PER_SECTOR];
} PACKED dir_sector;

int sector_start[] = {
    0, 21, 42, 63, 84, 105, 126, 147, 168, 189, 210, 231, 252, 273, 294,
    315, 336, 357, 376, 395, 414, 433, 452, 471, 490, 508, 526, 544, 562,
    580, 598, 615, 632, 649, 666, 683, 700, 717, 734, 751
};

char *file_type[] = {"DEL", "SEQ", "PRG", "USR", "REL", "???"};

static int memory_offset(track_sector *ts)
{
    return (ts->sector + sector_start[ts->track - 1]) * 256;
}

static int file_sector_size(dir_entry *de)
{
    return de->fs_sizel + (de->fs_sizeh * 256);
}

static int file_size(dir_entry *de)
{
    return file_sector_size(de) * 254;
}

static int next_dir_ts(dir_sector *ds, track_sector *ts)
{
    if (ds->dentry[0].next_dir_entry.track == 0)
        return 0;

    ts->track = ds->dentry[0].next_dir_entry.track;
    ts->sector = ds->dentry[0].next_dir_entry.sector;

    return 1;
}

static unsigned char simple_petscii(unsigned char c)
{
    switch(c) {
        case 0xab:
        case 0xad:
        case 0xae:
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xbd:
        case 0xc5:
        case 0xc9:
        case 0xca:
        case 0xcb:
        case 0xd2:
        case 0xd5:
        case 0xdb:
        case 0xed:
        case 0xee:
        case 0xf0:
        case 0xfd:
            return '+';

        case 0xc0:
        case 0xc3:
        case 0xc4:
        case 0xc6:
            return '-';

        case 0xc2:
        case 0xc7:
        case 0xc8:
        case 0xdd:
            return '|';

        case 0xa0:
            return ' ';

        default:
            return (isprint(c) ? c : '@');
    }
}

int validate_disk(int filesize)
{
    if (filesize != SIZE_35_TRACK && filesize != SIZE_35_TRACK_ERROR &&
            filesize != SIZE_40_TRACK && filesize != SIZE_40_TRACK_ERROR)
        return 0;

    return 1;
}

void showdisk(uint8_t *buffer)
{
    track_sector ts = {.track = 18, .sector = 0};
    int offset = memory_offset(&ts);

    bam_block *bam = (bam_block*)(&buffer[offset]);

    int free_sectors = 0;
    for (int i=0; i < BAM_NO_OF_ENTRIES; i++)
        free_sectors += bam->bam_entries[i].free_sectors;

    // Disk name
    printf("\"");
    for (int i=0; i < BAM_DISKNAME_LENGTH; i++)
        printf( "%c", simple_petscii(bam->diskname[i]));
    printf("\"");

    // Disk id
    for (int i=0; i < BAM_DISKINFO_LENGTH; i++)
        printf( "%c", simple_petscii(bam->diskinfo[i]));
    printf("\n");

    // Files
    ts.sector = 1;
    offset = memory_offset(&ts);
    dir_sector *ds = (dir_sector*)(&buffer[offset]);

    int valid = 1;
    while (valid) {
        for (int i=0; i < NO_OF_DIRENTRY_PER_SECTOR; i++) {
            dir_entry *de = (dir_entry*)(&ds->dentry[i]);

            if (de->filetype) {
                for (int j=0; j < FILENAME_LENGTH; j++)
                    printf( "%c", simple_petscii(de->filename[j]));

                // Actual filetype is the last three bits
                int ftype = de->filetype & ((1 << 3) - 1);
                if (ftype > 4)
                    ftype = (sizeof(file_type) / sizeof(file_type[0])) - 1;

                char *filetype = file_type[ftype];

                printf("  %-3s (0x%02X), %3d sectors, %6d bytes",
                        filetype, de->filetype, file_sector_size(de), file_size(de));
                printf(" (%02d,%02d)\n", de->filetrack, de->filesector);
            }
        }

        if (next_dir_ts(ds, &ts)) {
            offset = memory_offset(&ts);
            ds = (dir_sector*)(&buffer[offset]);
        } else
            valid = 0;
    }

    printf("Free: %d sectors, %d bytes\n", free_sectors, free_sectors * 254);
}
