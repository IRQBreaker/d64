#include "disk.h"
#include "util.h"

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

const int sector_start[] = {
    0, 21, 42, 63, 84, 105, 126, 147, 168, 189, 210, 231, 252, 273, 294,
    315, 336, 357, 376, 395, 414, 433, 452, 471, 490, 508, 526, 544, 562,
    580, 598, 615, 632, 649, 666, 683, 700, 717, 734, 751
};

const int sectors[] = {
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    19, 19, 19, 19, 19, 19, 19,
    18, 18, 18, 18, 18, 18,
    17, 17, 17, 17, 17
};

static int memory_offset(const track_sector *ts)
{
    return (ts->sector + sector_start[ts->track - 1]) * 256;
}

static int file_sector_size(const dir_entry *de)
{
    return de->fs_sizel + (de->fs_sizeh * 256);
}

static int file_size(const dir_entry *de)
{
    return file_sector_size(de) * 254;
}

static int next_dir_ts(const dir_sector *ds, track_sector *ts)
{
    if (ds->dentry[0].next_dir_entry.track == 0)
        return 0;

    ts->track = ds->dentry[0].next_dir_entry.track;
    ts->sector = ds->dentry[0].next_dir_entry.sector;

    return 1;
}

static int validate_disk(const int filesize)
{
    if (filesize != SIZE_35_TRACK && filesize != SIZE_35_TRACK_ERROR &&
            filesize != SIZE_40_TRACK && filesize != SIZE_40_TRACK_ERROR)
        return 0;

    return 1;
}

void disk(const uint8_t *buffer, const int size, const int baminfo)
{
    if (!validate_disk(size)) {
        fprintf(stderr, "Not a valid D64 disk image\n");
        return;
    }

    track_sector ts = {.track = 18, .sector = 0};

    bam_block *bam = (bam_block*)(&buffer[memory_offset(&ts)]);

    if (baminfo) {
        printf("BAM contents:\n");
        for (int i=0; i < BAM_NO_OF_ENTRIES; i++) {
            char bstring[25] = {0};
            int x = bam->bam_entries[i].bitmap[0];
            int y = bam->bam_entries[i].bitmap[1];
            int z = bam->bam_entries[i].bitmap[2];

            for (int j=0; j < 8; j++) {
                bstring[j] = ((x >> j) & 0x01) ? '.' : '*';
                bstring[j + 8] = ((y >> j) & 0x01) ? '.' : '*';
                bstring[j + 16] = ((z >> j) & 0x01) ? '.' : '*';
            }

            printf("Track %02d: ", i + 1);
            for (int k=0; k < sectors[i]; k++) {
                printf("%c", bstring[k]);
            }
            printf("\n");
        }
        printf("\n");
    }

    int free_sectors = 0;
    for (int i=0; i < BAM_NO_OF_ENTRIES; i++)
        free_sectors += bam->bam_entries[i].free_sectors;

    // Disk name
    printf("\"");
    for (int i=0; i < BAM_DISKNAME_LENGTH; i++)
        printf( "%c", isprint(pet_asc[bam->diskname[i]]) ?
                pet_asc[bam->diskname[i]] : ' ');
    printf("\"");

    // Disk id
    for (int i=0; i < BAM_DISKINFO_LENGTH; i++)
        printf( "%c", isprint(pet_asc[bam->diskinfo[i]]) ?
                pet_asc[bam->diskinfo[i]] : ' ');
    printf("\n");

    // Files
    ts.sector = 1;
    dir_sector *ds = (dir_sector*)(&buffer[memory_offset(&ts)]);

    int valid = 1;
    while (valid) {
        for (int i=0; i < NO_OF_DIRENTRY_PER_SECTOR; i++) {
            dir_entry *de = (dir_entry*)(&ds->dentry[i]);

            if (de->filetype) {
                for (int j=0; j < FILENAME_LENGTH; j++)
                    printf( "%c", isprint(pet_asc[de->filename[j]]) ?
                            pet_asc[de->filename[j]] : ' ');

                printf("  %-3s (0x%02X), %3d sectors, %6d bytes",
                        get_filetype(de->filetype), de->filetype,
                        file_sector_size(de), file_size(de));
                printf(" (%02d,%02d)\n", de->filetrack, de->filesector);
            }
        }

        if (next_dir_ts(ds, &ts)) {
            ds = (dir_sector*)(&buffer[memory_offset(&ts)]);
        } else
            valid = 0;
    }

    printf("Free: %d sectors, %d bytes\n", free_sectors, free_sectors * 254);
}
