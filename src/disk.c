#include "disk.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 256

// Image sizes and tracks
#define D64_TRACKS_35                35
#define D64_IMAGE_SIZE_35            174848L
#define D64_IMAGE_SIZE_35_ERR        175531L
#define D64_TRACKS_40                40
#define D64_IMAGE_SIZE_40            196608L
#define D64_IMAGE_SIZE_40_ERR        197376L
#define D64_TRACKS_42                42
#define D64_IMAGE_SIZE_42            205312L
#define D64_IMAGE_SIZE_42_ERR        206114L

// Layout constants
#define DIRECTORY_TRACK              18
#define BAM_TRACK                    18
#define BAM_SECTOR                   0
#define DIR_FIRST_SECTOR             1

#define BITS_PER_BYTE                8
#define MAX_CHAIN_STEPS              20000
#define END_OF_CHAIN_TRACK           0
#define DIR_NAME_SEPARATOR_SPACES    1
#define PRG_LOAD_ADDR_LEN            2
#define D64_35_FREE_CAPACITY_BLOCKS  664
#define D64_40_FREE_CAPACITY_BLOCKS  749

// PETSCII helpers
#define PETSCII_PAD                  0xA0
#define ASCII_SPACE                  0x20
#define ASCII_MIN_PRINTABLE          0x20

// DOS type and Disk ID lengths (not including NUL)
#define DOS_TYPE_LEN                 2
#define DISK_ID_LEN                  2

// PRG format
#define PRG_LOAD_ADDR_LO_OFF         2
#define PRG_LOAD_ADDR_HI_OFF         3

// Directory layout
#define DIR_ENTRY_SIZE               32
#define DIR_ENTRIES_PER_SECTOR       8
#define DIR_FILETYPE_OFF             2
#define DIR_START_TRACK_OFF          3
#define DIR_START_SECTOR_OFF         4
#define DIR_FILENAME_OFF             5
#define DIR_FILENAME_LEN             16
#define DIR_FILESIZE_LO_OFF          0x1E
#define DIR_FILESIZE_HI_OFF          0x1F

// Sector link
#define SECTOR_LINK_TRACK_OFF        0
#define SECTOR_LINK_SECTOR_OFF       1

// File types + flags
#define FILETYPE_MASK                0x07
#define FILETYPE_DEL                 0x00
#define FILETYPE_SEQ                 0x01
#define FILETYPE_PRG                 0x02
#define FILETYPE_USR                 0x03
#define FILETYPE_REL                 0x04
#define FILE_FLAG_LOCKED             0x40
#define FILE_FLAG_CLOSED             0x80

#define FILE_DATA_BYTES_PER_SECTOR   254

// BAM layout
#define BAM_ENTRIES_BASE             0x04   // 4 bytes per track
#define BAM_ENTRY_STRIDE             4
#define BAM_ENTRY_FREECOUNT_OFF      0
#define BAM_ENTRY_BITMAP1_OFF        1
#define BAM_ENTRY_BITMAP2_OFF        2
#define BAM_ENTRY_BITMAP3_OFF        3
#define BAM_DISK_NAME_OFF            0x90
#define BAM_DISK_NAME_LEN            16
#define BAM_DISK_ID_OFF              0xA2
#define BAM_DOS_TYPE_OFF             0xA5
// Extended BAM for tracks 36-40
#define BAM_DOLPHIN_BASE             0xAC
#define BAM_SPEED_BASE               0xC0

static int g_total_tracks = D64_TRACKS_35;
static const uint8_t *g_image = NULL;
static size_t g_image_size = 0;
static size_t g_data_bytes = 0;   // size of main data area (without error bytes)

static int sectors_per_track(int track)
{
    if (track < 1) {
        return 0;
    }

    if (track <= 17) {
        return 21;
    }

    if (track <= 24) {
        return 19;
    }

    if (track <= 30) {
        return 18;
    }

    if (track <= 42) {
        return 17;
    }

    return 0;
}

static long ts_offset(int track, int sector)
{
    if (track < 1 || track > g_total_tracks) {
        return -1;
    }

    long off = 0;

    for (int t = 1; t < track; ++t) {
        off += (long)sectors_per_track(t) * SECTOR_SIZE;
    }

    int spt = sectors_per_track(track);

    if (sector < 0 || sector >= spt) {
        return -1;
    }

    off += (long)sector * SECTOR_SIZE;

    return off;
}

static int read_sector(int track, int sector, const uint8_t **out)
{
    long off = ts_offset(track, sector);

    if (off < 0) {
        return -1;
    }

    if (!g_image || (size_t)(off + SECTOR_SIZE) > g_data_bytes) {
        return -1;
    }

    *out = g_image + off;

    return 0;
}

static int get_bam_entry(const uint8_t *bam, int track, int *free_cnt, uint8_t *b1, uint8_t *b2, uint8_t *b3)
{
    if (track >= 1 && track <= 35) {
        int base = BAM_ENTRIES_BASE + (track - 1) * BAM_ENTRY_STRIDE;

        if (free_cnt) {
            *free_cnt = bam[base + BAM_ENTRY_FREECOUNT_OFF];
        }

        if (b1) {
            *b1 = bam[base + BAM_ENTRY_BITMAP1_OFF];
        }

        if (b2) {
            *b2 = bam[base + BAM_ENTRY_BITMAP2_OFF];
        }

        if (b3) {
            *b3 = bam[base + BAM_ENTRY_BITMAP3_OFF];
        }

        return 1;
    } else if (track >= 36 && track <= 40) {
        int idx = track - 36;
        int base_d = BAM_DOLPHIN_BASE + idx * BAM_ENTRY_STRIDE;
        int base_s = BAM_SPEED_BASE   + idx * BAM_ENTRY_STRIDE;
        int fc_d = bam[base_d + BAM_ENTRY_FREECOUNT_OFF];
        uint8_t d1 = bam[base_d + BAM_ENTRY_BITMAP1_OFF];
        uint8_t d2 = bam[base_d + BAM_ENTRY_BITMAP2_OFF];
        uint8_t d3 = bam[base_d + BAM_ENTRY_BITMAP3_OFF];

        if (fc_d || d1 || d2 || d3) {
            if (free_cnt) {
                *free_cnt = fc_d;
            }

            if (b1) {
                *b1 = d1;
            }

            if (b2) {
                *b2 = d2;
            }

            if (b3) {
                *b3 = d3;
            }

            return 1;
        }

        int fc_s = bam[base_s + BAM_ENTRY_FREECOUNT_OFF];
        uint8_t s1 = bam[base_s + BAM_ENTRY_BITMAP1_OFF];
        uint8_t s2 = bam[base_s + BAM_ENTRY_BITMAP2_OFF];
        uint8_t s3 = bam[base_s + BAM_ENTRY_BITMAP3_OFF];

        if (fc_s || s1 || s2 || s3) {
            if (free_cnt) {
                *free_cnt = fc_s;
            }

            if (b1) {
                *b1 = s1;
            }

            if (b2) {
                *b2 = s2;
            }

            if (b3) {
                *b3 = s3;
            }

            return 1;
        }
    }

    return 0;
}

static long compute_file_size_bytes(int start_track, int start_sector)
{
    if (start_track <= 0 || start_sector < 0) {
        return 0;
    }

    const uint8_t *sec = NULL;
    long total = 0;
    int track = start_track;
    int sector = start_sector;

    for (int steps = 0; steps < MAX_CHAIN_STEPS; ++steps) {
        if (read_sector(track, sector, &sec) != 0) {
            break;
        }

        int nt = sec[SECTOR_LINK_TRACK_OFF];
        int ns = sec[SECTOR_LINK_SECTOR_OFF];

        if (nt == END_OF_CHAIN_TRACK) {
            if (ns >= 0 && ns <= FILE_DATA_BYTES_PER_SECTOR) {
                total += ns;
            }
            break;
        }

        total += FILE_DATA_BYTES_PER_SECTOR;
        track = nt;
        sector = ns;
    }

    return total;
}

static void detect_image_layout(void)
{
    if (g_image_size == D64_IMAGE_SIZE_35) {
        g_total_tracks = D64_TRACKS_35;
        g_data_bytes = D64_IMAGE_SIZE_35;
        return;
    }

    if (g_image_size == D64_IMAGE_SIZE_40) {
        g_total_tracks = D64_TRACKS_40;
        g_data_bytes = D64_IMAGE_SIZE_40;
        return;
    }

    if (g_image_size == D64_IMAGE_SIZE_42) {
        g_total_tracks = D64_TRACKS_42;
        g_data_bytes = D64_IMAGE_SIZE_42;
        return;
    }

    if (g_image_size == D64_IMAGE_SIZE_35_ERR) {
        g_total_tracks = D64_TRACKS_35;
        g_data_bytes = D64_IMAGE_SIZE_35;
        return;
    }

    if (g_image_size == D64_IMAGE_SIZE_40_ERR) {
        g_total_tracks = D64_TRACKS_40;
        g_data_bytes = D64_IMAGE_SIZE_40;
        return;
    }

    if (g_image_size == D64_IMAGE_SIZE_42_ERR) {
        g_total_tracks = D64_TRACKS_42;
        g_data_bytes = D64_IMAGE_SIZE_42;
        return;
    }

    // Fallback: assume 35 tracks, clamp to buffer size
    g_total_tracks = D64_TRACKS_35;
    g_data_bytes = g_image_size;
}

static int read_bam(const uint8_t **bam)
{
    return read_sector(BAM_TRACK, BAM_SECTOR, bam) == 0;
}

static void petscii_to_ascii_trim(const uint8_t *in, size_t len, char *out, size_t outsz)
{
    size_t j = 0;

    for (size_t i = 0; i < len && j + 1 < outsz; ++i) {
        uint8_t c = in[i];

        if (c == PETSCII_PAD) {
            c = ASCII_SPACE;
        }

        if (c < ASCII_MIN_PRINTABLE) {
            c = '?';
        }

        out[j++] = (char)c;
    }

    while (j > 0 && out[j-1] == ' ') {
        j--;
    }

    out[j] = '\0';
}

// Map PETSCII directory name bytes to display ASCII, preserving 16 chars.
// - Pads (0xA0) -> space
// - ASCII letters -> lowercase
// - Known graphics -> ASCII approximations (B, ., M, N, V, etc.)
static void petscii_dirname_16(const uint8_t *in, char *out /* size >= 17 */)
{
    for (int i = 0; i < DIR_FILENAME_LEN; ++i) {
        uint8_t c = in[i];

        char o = '?';

        if (c == PETSCII_PAD) {
            o = ' ';
        } else if (c >= 0x20 && c <= 0x7E) {
            // ASCII printable. Lowercase letters to match desired style.
            if (c >= 'A' && c <= 'Z') {
                o = (char)(c - 'A' + 'a');
            } else {
                o = (char)c;
            }
        } else {
            // Graphics and special PETSCII seen in directory art.
            switch (c) {
                // Dots / background
                case 0xAD:
                case 0xAE:
                case 0xAF:
                case 0xB0:
                case 0xB1:
                case 0xB2:
                case 0xB3:
                case 0xBD:
                case 0xC0:
                case 0xDB:
                    o = '.'; break;

                // Block / border
                case 0xC2:
                    o = 'B';
                    break;  // solid block

                // Corners/edges and markers
                case 0xD4:
                    o = 'T';
                    break;  // top

                case 0xD9:
                    o = 'Y';
                    break;  // top-right/right marker

                case 0xCA:
                    o = 'J';
                    break;  // bottom-left

                case 0xCB:
                    o = 'K';
                    break;  // bottom-right

                case 0xD5:
                    o = 'U';
                    break;  // up

                case 0xC4:
                    o = 'D';
                    break;  // horizontal line

                case 0xC9:
                    o = 'I';
                    break;  // right marker

                case 0xC6:
                    o = 'F';
                    break;

                case 0xC7:
                    o = 'G';
                    break;

                case 0xC8:
                    o = 'H';
                    break;

                case 0xCF:
                    o = 'O';
                    break;

                case 0xD3:
                    o = 'S';
                    break;

                case 0xCD:
                    o = 'M';
                    break;

                case 0xCE:
                    o = 'N';
                    break;

                case 0xD6:
                    o = 'V';
                    break;

                default:
                    o = '.'; // fall back to dot for other graphics
                    break;
            }
        }

        out[i] = o;
    }

    out[DIR_FILENAME_LEN] = '\0';
}

static void print_header_from_bam(const uint8_t *bam)
{
    char disk_name[BAM_DISK_NAME_LEN + 1];
    char id_ver_type[DISK_ID_LEN + DOS_TYPE_LEN + 1 + 1];

    petscii_to_ascii_trim(&bam[BAM_DISK_NAME_OFF], BAM_DISK_NAME_LEN, disk_name, sizeof(disk_name));

    // Compose id + version + type (e.g., "rules") all lowercased, no spaces
    uint8_t id0 = bam[BAM_DISK_ID_OFF + 0];
    uint8_t id1 = bam[BAM_DISK_ID_OFF + 1];
    uint8_t ver = bam[BAM_DOS_TYPE_OFF - 1];
    uint8_t dt0 = bam[BAM_DOS_TYPE_OFF + 0];
    uint8_t dt1 = bam[BAM_DOS_TYPE_OFF + 1];

    id_ver_type[0] = (char)id0;
    id_ver_type[1] = (char)id1;
    id_ver_type[2] = (char)ver;
    id_ver_type[3] = (char)dt0;
    id_ver_type[4] = (char)dt1;
    id_ver_type[5] = '\0';

    for (char *p = id_ver_type; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = (char)(*p - 'A' + 'a');
        }
    }

    char padded_name[BAM_DISK_NAME_LEN + 1];

    size_t dnlen = strlen(disk_name);
    if (dnlen > BAM_DISK_NAME_LEN) {
        dnlen = BAM_DISK_NAME_LEN;
    }

    memset(padded_name, ' ', BAM_DISK_NAME_LEN);
    memcpy(padded_name, disk_name, dnlen);

    for (int i = 0; i < BAM_DISK_NAME_LEN; ++i) {
        if (padded_name[i] >= 'A' && padded_name[i] <= 'Z') {
            padded_name[i] = (char)(padded_name[i]-'A'+'a');
        }
    }

    padded_name[BAM_DISK_NAME_LEN] = '\0';

    printf("0 \"%s\" %s\n", padded_name, id_ver_type);
}

static int list_directory(const uint8_t *bam, int show_sizes)
{
    (void)bam;
    const uint8_t *sec = NULL;
    int track = DIRECTORY_TRACK;
    int sector = DIR_FIRST_SECTOR;
    int total_blocks = 0;

    while (track != END_OF_CHAIN_TRACK) {
        if (read_sector(track, sector, &sec) != 0) {
            fprintf(stderr, "Failed to read directory sector %d/%d.\n", track, sector);
            break;
        }

        int next_track = sec[SECTOR_LINK_TRACK_OFF];
        int next_sector = sec[SECTOR_LINK_SECTOR_OFF];

        for (int i = 0; i < DIR_ENTRIES_PER_SECTOR; ++i) {
            int off = i * DIR_ENTRY_SIZE;
            uint8_t file_type = sec[off + DIR_FILETYPE_OFF];

            if (file_type == 0x00) {
                continue;
            }

            char name[DIR_FILENAME_LEN + 1];
            petscii_dirname_16(&sec[off + DIR_FILENAME_OFF], name);

            // Entries consisting of only spaces can be skipped
            int all_blank = 1;

            for (int k = 0; k < DIR_FILENAME_LEN; ++k) {
                if (name[k] != ' ') {
                    all_blank = 0;
                    break;
                }
            }

            if (all_blank) {
                continue;
            }

            int blocks = sec[off + DIR_FILESIZE_LO_OFF] + (sec[off + DIR_FILESIZE_HI_OFF] << 8);

            const char *type_base = "???";

            switch (file_type & FILETYPE_MASK) {
                case FILETYPE_DEL:
                    type_base = "del";
                    break;

                case FILETYPE_SEQ:
                    type_base = "seq";
                    break;

                case FILETYPE_PRG:
                    type_base = "prg";
                    break;

                case FILETYPE_USR:
                    type_base = "usr";
                    break;

                case FILETYPE_REL:
                    type_base = "rel";
                    break;

                default:
                    type_base = "???";
                    break;
            }

            char type_marked[8];
            int ti = 0;

            if (file_type & FILE_FLAG_LOCKED) {
                type_marked[ti++] = '>';
            }

            size_t tlen = strlen(type_base);
            memcpy(&type_marked[ti], type_base, tlen);
            ti += (int)tlen;

            if ((file_type & FILE_FLAG_CLOSED) == 0) {
                type_marked[ti++] = '*';
            }

            type_marked[ti] = '\0';

            printf("%-5d\"%s\" ", blocks, name);

            printf("%s", type_marked);

            if (show_sizes) {
                int start_track = sec[off + DIR_START_TRACK_OFF];
                int start_sector = sec[off + DIR_START_SECTOR_OFF];
                long bytes = compute_file_size_bytes(start_track, start_sector);

                printf("  %ld bytes", bytes);

                if ((file_type & FILETYPE_MASK) == FILETYPE_PRG && start_track > 0) {
                    const uint8_t *first_sec = NULL;

                    if (read_sector(start_track, start_sector, &first_sec) == 0 && bytes >= PRG_LOAD_ADDR_LEN) {
                        uint16_t load = (uint16_t)first_sec[PRG_LOAD_ADDR_LO_OFF] | ((uint16_t)first_sec[PRG_LOAD_ADDR_HI_OFF] << 8);
                        long data_bytes = bytes - 2;

                        if (data_bytes < 0) {
                            data_bytes = 0;
                        }

                        uint32_t end_addr_inclusive = (uint32_t)load + (uint32_t)(data_bytes ? (data_bytes - 1) : 0);
                        printf("  $%04x-$%04x", load, (unsigned)end_addr_inclusive);
                    }
                }
            }

            printf("\n");
            total_blocks += blocks;
        }

        track = next_track;
        sector = next_sector;
    }

    return total_blocks;
}

static int compute_free_blocks(const uint8_t *bam, int total_file_blocks)
{
    (void)total_file_blocks;

    int free_blocks = 0;

    for (int t = 1; t <= g_total_tracks; ++t) {
        int free = 0;
        uint8_t b1 = 0, b2 = 0, b3 = 0;

        if (get_bam_entry(bam, t, &free, &b1, &b2, &b3)) {
            // Use the per-track free count reported in BAM
            free_blocks += free;
        } else {
            // Fallback: bit-count if BAM entry unavailable
            int spt = sectors_per_track(t);
            for (int s = 0; s < spt; ++s) {
                uint8_t bb = (s < BITS_PER_BYTE) ? b1 : (s < 2 * BITS_PER_BYTE) ? b2 : b3;

                if (bb & (1u << (s & (BITS_PER_BYTE - 1)))) {
                    free_blocks++;
                }
            }
        }
    }

    // c1541 "dir" output appears to exclude any free sectors on the
    // directory track (track 18) from the free-blocks summary.
    // Adjust to match that behavior.
    if (DIRECTORY_TRACK >= 1 && DIRECTORY_TRACK <= g_total_tracks) {
        int dir_free = 0;
        uint8_t b1 = 0, b2 = 0, b3 = 0;

        if (get_bam_entry(bam, DIRECTORY_TRACK, &dir_free, &b1, &b2, &b3)) {
            free_blocks -= dir_free;

            if (free_blocks < 0) {
                free_blocks = 0;
            }
        }
    }

    return free_blocks;
}

static void print_bam_summary(const uint8_t *bam)
{
    printf("BAM:\n");
    for (int t = 1; t <= g_total_tracks; ++t) {
        int free = 0;
        uint8_t b1 = 0, b2 = 0, b3 = 0;

        if (!get_bam_entry(bam, t, &free, &b1, &b2, &b3)) {
            free = 0;
            b1 = b2 = b3 = 0;
        }

        printf("T%02d free=%3d map=%02x %02x %02x\n", t, free, b1, b2, b3);
    }
}

void disk(const uint8_t *buffer, const int size, const int baminfo)
{
    if (!buffer || size <= 0) {
        fprintf(stderr, "Not a valid D64 disk image\n");
        return;
    }

    g_image = buffer;
    g_image_size = (size_t)size;

    detect_image_layout();

    const uint8_t *bam = NULL;
    if (!read_bam(&bam)) {
        fprintf(stderr, "Failed to read BAM sector.\n");
        return;
    }

    print_header_from_bam(bam);

    int total_blocks = list_directory(bam, 1);
    int free_blocks = compute_free_blocks(bam, total_blocks);

    printf("%d blocks free.\n", free_blocks);

    if (baminfo) {
        print_bam_summary(bam);
    }
}
