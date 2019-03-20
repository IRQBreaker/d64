#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
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
  track_sector next_de;
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
  dir_entry sector[NO_OF_DIRENTRY_PER_SECTOR];
} PACKED dir_sector;

unsigned char pet_asc[256] = {
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x14,0x09,0x0d,0x11,0x93,0x0a,0x0e,0x0f,
  0x10,0x0b,0x12,0x13,0x08,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
  0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
  0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
  0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
  0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
  0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
  0x90,0x91,0x92,0x0c,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
  0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
  0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
  0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
  0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
  0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf
};

int memory_offset(track_sector *ts)
{
  int offset = -1;

  if (ts->track >= 1 && ts->track <=17)
    offset = ts->sector * 256;
  else if (ts->track >= 18 && ts->track <= 24)
    offset = (ts->sector * 256) + ((17 * 21) * 256);
  else if (ts->track >= 25 && ts->track <=30)
    offset = (ts->sector * 256) + ((7 * 19) * 256);
  else if (ts->track >= 31 && ts->track <=35)
    offset = (ts->sector * 256) + ((6 * 18) * 256);

  return offset;
}

int file_sector_size(dir_entry *de)
{
  return de->fs_sizel + (de->fs_sizeh * 256);
}

int file_size(dir_entry *de)
{
  return file_sector_size(de) * 254;
}

int next_dir_ts(dir_sector *ds, track_sector *ts)
{
  if (ds->sector[0].next_de.track == 0)
    return 0;

  ts->track = ds->sector[0].next_de.track;
  ts->sector = ds->sector[0].next_de.sector;

  return 1;
}

char* file_type(int filetype)
{
  switch (filetype & ((1 << 3) - 1)) {
    case FILETYPE_DEL:
      return "DEL";
    case FILETYPE_SEQ:
      return "SEQ";
    case FILETYPE_PRG:
      return "PRG";
    case FILETYPE_USR:
      return "USR";
    case FILETYPE_REL:
      return "REL";
    default:
      return "???";
  }
}

int validate_disk(int filesize)
{
  if (filesize != SIZE_35_TRACK && filesize != SIZE_35_TRACK_ERROR &&
      filesize != SIZE_40_TRACK && filesize != SIZE_40_TRACK_ERROR)
    return 0;

  return 1;
}

void showdisk(char *buffer)
{
  track_sector ts = {.track = 18, .sector = 0};
  int offset = memory_offset(&ts);

  bam_block *bam = (bam_block*)(&buffer[offset]);

  int free_sectors = 0;
  for (int i=0; i < BAM_NO_OF_ENTRIES; i++)
    free_sectors += bam->bam_entries[i].free_sectors;

  // Disk name
  printf("\"");
  for (int i=0; i < BAM_DISKNAME_LENGTH; i++) {
    if (isprint(pet_asc[bam->diskname[i]]))
      printf( "%c", pet_asc[bam->diskname[i]]);
    else
      printf(" ");
  }
  printf("\"");

  // Disk id
  for (int i=0; i < BAM_DISKINFO_LENGTH; i++) {
    if (isprint(pet_asc[bam->diskinfo[i]]))
      printf( "%c", pet_asc[bam->diskinfo[i]]);
    else
      printf(" ");
  }
  printf("\n");

  // Files
  ts.sector = 1;
  offset = memory_offset(&ts);
  dir_sector *ds = (dir_sector*)(&buffer[offset]);

  int valid = 1;
  while (valid) {
    for (int i=0; i < NO_OF_DIRENTRY_PER_SECTOR; i++) {
      dir_entry *de = (dir_entry*)(&ds->sector[i]);

      if (de->filetype) {
        for (int j=0; j < FILENAME_LENGTH; j++) {
          if (isprint(pet_asc[de->filename[j]]))
            printf( "%c", pet_asc[de->filename[j]]);
          else
            printf(" ");
        }

        printf("  %-3s (0x%02X), %3d sectors, %6d bytes",
            file_type(de->filetype), de->filetype,
            file_sector_size(de),
            file_size(de));
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

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments\n");
    return EXIT_FAILURE;
  }

  int fd = open(argv[1], O_RDONLY);
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

  if (!validate_disk(st.st_size)) {
    fprintf(stderr, "Not a valid D64 file\n");
    close(fd);
    return EXIT_FAILURE;
  }

  char *buffer = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (buffer == MAP_FAILED) {
    perror("Error");
    close(fd);
    return EXIT_FAILURE;
  }

  showdisk(buffer);

  munmap(buffer, st.st_size);
  close(fd);

  return EXIT_SUCCESS;
}
