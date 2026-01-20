#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static long file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return (long)st.st_size;
}

static int usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s --out <path> --stage2 <path> --kernel <path> --prog1 <path> --prog2 <path> --note <path>\n",
            prog);
    return 1;
}

static void write_entry(unsigned char *dst, const char *name,
                        unsigned short start_cluster) {
    size_t i;
    for (i = 0; i < 8; i++) {
        dst[i] = (i < strlen(name)) ? (unsigned char)name[i] : (unsigned char)' ';
    }
    dst[8] = (unsigned char)(start_cluster & 0xFF);
    dst[9] = (unsigned char)((start_cluster >> 8) & 0xFF);
    dst[10] = 0;
    dst[11] = 0;
    dst[12] = 0;
    dst[13] = 0;
    dst[14] = 0;
    dst[15] = 0;
}

int main(int argc, char **argv) {
    const char *out = NULL;
    const char *stage2 = NULL;
    const char *kernel = NULL;
    const char *prog1 = NULL;
    const char *prog2 = NULL;
    const char *note = NULL;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out = argv[++i];
        } else if (strcmp(argv[i], "--stage2") == 0 && i + 1 < argc) {
            stage2 = argv[++i];
        } else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) {
            kernel = argv[++i];
        } else if (strcmp(argv[i], "--prog1") == 0 && i + 1 < argc) {
            prog1 = argv[++i];
        } else if (strcmp(argv[i], "--prog2") == 0 && i + 1 < argc) {
            prog2 = argv[++i];
        } else if (strcmp(argv[i], "--note") == 0 && i + 1 < argc) {
            note = argv[++i];
        } else {
            return usage(argv[0]);
        }
    }

    if (!out || !stage2 || !kernel || !prog1 || !prog2 || !note) {
        return usage(argv[0]);
    }

    long stage2_size = file_size(stage2);
    long kernel_size = file_size(kernel);
    long prog1_size = file_size(prog1);
    long prog2_size = file_size(prog2);
    long note_size = file_size(note);
    if (stage2_size < 0 || kernel_size < 0 || prog1_size < 0 || prog2_size < 0 || note_size < 0) {
        fprintf(stderr, "mkfs: stat failed: %s\n", strerror(errno));
        return 1;
    }

    unsigned short stage2_sect = (unsigned short)((stage2_size + 511) / 512);
    unsigned short kernel_sect = (unsigned short)((kernel_size + 511) / 512);
    unsigned short prog1_sect = (unsigned short)((prog1_size + 511) / 512);
    unsigned short prog2_sect = (unsigned short)((prog2_size + 511) / 512);
    unsigned short note_sect = (unsigned short)((note_size + 511) / 512);

    const unsigned int fat_lba = 2;
    const unsigned int root_lba = 3;
    const unsigned int data_lba = 4;

    unsigned short stage2_start = 2;
    unsigned short kernel_start = (unsigned short)(stage2_start + stage2_sect);
    unsigned short prog1_start = (unsigned short)(kernel_start + kernel_sect);
    unsigned short prog2_start = (unsigned short)(prog1_start + prog1_sect);
    unsigned short note_start = (unsigned short)(prog2_start + prog2_sect);
    if ((unsigned long)note_start + note_sect >= 256) {
        fprintf(stderr, "mkfs: too many sectors for 1-sector FAT\n");
        return 1;
    }

    unsigned char fs[512 * 3];
    memset(fs, 0, sizeof(fs));

    fs[0] = 'O';
    fs[1] = 'S';
    fs[2] = 'F';
    fs[3] = 'T';
    fs[4] = 0x00;
    fs[5] = 0x02; /* 512 */
    fs[6] = 1;    /* sectors_per_cluster */
    fs[7] = 0;
    fs[8] = (unsigned char)(fat_lba & 0xFF);
    fs[9] = (unsigned char)((fat_lba >> 8) & 0xFF);
    fs[10] = (unsigned char)((fat_lba >> 16) & 0xFF);
    fs[11] = (unsigned char)((fat_lba >> 24) & 0xFF);
    fs[12] = 1;
    fs[16] = (unsigned char)(root_lba & 0xFF);
    fs[17] = (unsigned char)((root_lba >> 8) & 0xFF);
    fs[18] = (unsigned char)((root_lba >> 16) & 0xFF);
    fs[19] = (unsigned char)((root_lba >> 24) & 0xFF);
    fs[20] = 1;
    fs[24] = (unsigned char)(data_lba & 0xFF);
    fs[25] = (unsigned char)((data_lba >> 8) & 0xFF);
    fs[26] = (unsigned char)((data_lba >> 16) & 0xFF);
    fs[27] = (unsigned char)((data_lba >> 24) & 0xFF);

    unsigned char *fat = fs + 512;
    fat[0] = 0xFF;
    fat[1] = 0xFF;
    fat[2] = 0xFF;
    fat[3] = 0xFF;

    unsigned short i_cluster;
    for (i_cluster = 0; i_cluster < stage2_sect; i_cluster++) {
        unsigned short clus = (unsigned short)(stage2_start + i_cluster);
        unsigned short next = (unsigned short)((i_cluster + 1 == stage2_sect) ? 0xFFFF
                                                                              : clus + 1);
        fat[clus * 2] = (unsigned char)(next & 0xFF);
        fat[clus * 2 + 1] = (unsigned char)((next >> 8) & 0xFF);
    }

    for (i_cluster = 0; i_cluster < kernel_sect; i_cluster++) {
        unsigned short clus = (unsigned short)(kernel_start + i_cluster);
        unsigned short next = (unsigned short)((i_cluster + 1 == kernel_sect) ? 0xFFFF
                                                                              : clus + 1);
        fat[clus * 2] = (unsigned char)(next & 0xFF);
        fat[clus * 2 + 1] = (unsigned char)((next >> 8) & 0xFF);
    }

    for (i_cluster = 0; i_cluster < prog1_sect; i_cluster++) {
        unsigned short clus = (unsigned short)(prog1_start + i_cluster);
        unsigned short next = (unsigned short)((i_cluster + 1 == prog1_sect) ? 0xFFFF
                                                                            : clus + 1);
        fat[clus * 2] = (unsigned char)(next & 0xFF);
        fat[clus * 2 + 1] = (unsigned char)((next >> 8) & 0xFF);
    }

    for (i_cluster = 0; i_cluster < prog2_sect; i_cluster++) {
        unsigned short clus = (unsigned short)(prog2_start + i_cluster);
        unsigned short next = (unsigned short)((i_cluster + 1 == prog2_sect) ? 0xFFFF
                                                                            : clus + 1);
        fat[clus * 2] = (unsigned char)(next & 0xFF);
        fat[clus * 2 + 1] = (unsigned char)((next >> 8) & 0xFF);
    }

    for (i_cluster = 0; i_cluster < note_sect; i_cluster++) {
        unsigned short clus = (unsigned short)(note_start + i_cluster);
        unsigned short next = (unsigned short)((i_cluster + 1 == note_sect) ? 0xFFFF
                                                                            : clus + 1);
        fat[clus * 2] = (unsigned char)(next & 0xFF);
        fat[clus * 2 + 1] = (unsigned char)((next >> 8) & 0xFF);
    }

    unsigned char *root = fs + 1024;
    write_entry(root + 0, "STAGE2", stage2_start);
    write_entry(root + 16, "KERNEL", kernel_start);
    write_entry(root + 32, "U1", prog1_start);
    write_entry(root + 48, "U2", prog2_start);
    write_entry(root + 64, "NOTE", note_start);

    FILE *f = fopen(out, "wb");
    if (!f) {
        fprintf(stderr, "mkfs: fopen failed: %s\n", strerror(errno));
        return 1;
    }
    if (fwrite(fs, 1, sizeof(fs), f) != sizeof(fs)) {
        fprintf(stderr, "mkfs: write failed\n");
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}
