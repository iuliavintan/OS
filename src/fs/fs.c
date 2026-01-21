#include "fs.h"
#include "ata.h"
#include "../utils/util.h"
#include "../utils/vga.h"

#define FS_MAGIC 0x5446534fU /* "OSFT" */
#define FS_HDR_LBA 1
#define FS_SECTOR_SIZE 512

typedef struct __attribute__((packed)) fs_header {
    uint32_t magic;
    uint16_t sector_size;
    uint8_t sectors_per_cluster;
    uint8_t reserved;
    uint32_t fat_lba;
    uint16_t fat_sectors;
    uint16_t pad1;
    uint32_t root_lba;
    uint16_t root_sectors;
    uint16_t pad2;
    uint32_t data_lba;
} fs_header;

typedef struct __attribute__((packed)) fs_dirent {
    char name[8];
    uint16_t start_cluster;
    uint8_t pad[6];
} fs_dirent;

static fs_header g_hdr;
static uint8_t g_hdr_sector[FS_SECTOR_SIZE];
static uint8_t g_fat[FS_SECTOR_SIZE];
static uint8_t g_root[FS_SECTOR_SIZE];
static int g_fs_ready = 0;

static int name_match(const char *name8, const char entry[8]) {
    for (int i = 0; i < 8; i++) {
        char c = name8[i];
        if (c == 0) {
            c = ' ';
        }
        if (entry[i] != c) {
            return 0;
        }
    }
    return 1;
}

static int find_start_cluster(const char *name8, uint16_t *out_start) {
    fs_dirent *ents = (fs_dirent *)g_root;
    for (uint32_t i = 0; i < (FS_SECTOR_SIZE / sizeof(fs_dirent)); i++) {
        if (name_match(name8, ents[i].name)) {
        if (out_start != NULL) {
            *out_start = ents[i].start_cluster;
        }
            return 0;
        }
    }
    return -1;
}

static int chain_length(uint16_t start, uint16_t *out_len) {
    uint16_t *fat = (uint16_t *)g_fat;
    uint16_t count = 0;
    uint16_t cluster = start;
    while (cluster != 0xFFFF) {
        if (cluster < 2 || cluster >= 256) {
            return -1;
        }
        count++;
        cluster = fat[cluster];
        if (count > 254) {
            return -1;
        }
    }
    if (out_len != NULL) {
        *out_len = count;
    }
    return 0;
}

int fs_init(void) {
    g_fs_ready = 0;
    if (ata_pio_read(FS_HDR_LBA, 1, g_hdr_sector) != 0) {
        return -1;
    }
    memcpy(&g_hdr, g_hdr_sector, sizeof(g_hdr));
    if (g_hdr.magic != FS_MAGIC || g_hdr.sector_size != FS_SECTOR_SIZE) {
        return -1;
    }
    if (g_hdr.fat_sectors != 1 || g_hdr.root_sectors != 1 || g_hdr.sectors_per_cluster != 1) {
        return -1;
    }
    if (ata_pio_read(g_hdr.fat_lba, 1, g_fat) != 0) {
        return -1;
    }
    if (ata_pio_read(g_hdr.root_lba, 1, g_root) != 0) {
        return -1;
    }
    g_fs_ready = 1;
    return 0;
}

int fs_load_file(const char *name8, void *dest, uint32_t max_sectors, uint32_t *out_sectors) {
    uint16_t start = 0;
    if (find_start_cluster(name8, &start) != 0) {
        return -1;
    }
    if (start < 2) {
        return -1;
    }

    uint16_t *fat = (uint16_t *)g_fat;
    uint8_t *dst = (uint8_t *)dest;
    uint32_t count = 0;
    uint16_t cluster = start;
    while (cluster != 0xFFFF) {
        if (cluster < 2 || cluster >= 256) {
            return -1;
        }
        if (count >= max_sectors) {
            return -1;
        }
        uint32_t lba = g_hdr.data_lba + (uint32_t)(cluster - 2);
        if (ata_pio_read(lba, 1, dst) != 0) {
            return -1;
        }
        dst += FS_SECTOR_SIZE;
        count++;
        cluster = fat[cluster];
    }
    if (out_sectors != NULL) {
        *out_sectors = count;
    }
    return 0;
}

int fs_file_capacity(const char *name8, uint32_t *out_bytes) {
    if (g_fs_ready == 0 || out_bytes == NULL) {
        return -1;
    }
    uint16_t start = 0;
    if (find_start_cluster(name8, &start) != 0 || start < 2) {
        return -1;
    }
    uint16_t count = 0;
    if (chain_length(start, &count) != 0) {
        return -1;
    }
    *out_bytes = (uint32_t)count * FS_SECTOR_SIZE;
    return 0;
}

int fs_write_file(const char *name8, const void *src, uint32_t bytes) {
    if (g_fs_ready == 0 || src == NULL) {
        return -1;
    }
    uint16_t start = 0;
    if (find_start_cluster(name8, &start) != 0 || start < 2) {
        return -1;
    }
    uint16_t count = 0;
    if (chain_length(start, &count) != 0) {
        return -1;
    }
    uint32_t max_bytes = (uint32_t)count * FS_SECTOR_SIZE;
    if (bytes > max_bytes) {
        return -1;
    }

    const uint8_t *src_bytes = (const uint8_t *)src;
    uint16_t *fat = (uint16_t *)g_fat;
    uint16_t cluster = start;
    uint32_t remaining = bytes;
    uint8_t sector[FS_SECTOR_SIZE];
    for (uint16_t i = 0; i < count; i++) {
        uint32_t lba = g_hdr.data_lba + (uint32_t)(cluster - 2);
        uint32_t copy = FS_SECTOR_SIZE;
        if (remaining < FS_SECTOR_SIZE) {
            copy = remaining;
        }
        if (copy > 0) {
            memcpy(sector, src_bytes, copy);
            if (copy < FS_SECTOR_SIZE) {
                memset(sector + copy, 0, FS_SECTOR_SIZE - copy);
            }
        } else {
            memset(sector, 0, FS_SECTOR_SIZE);
        }
        if (ata_pio_write(lba, 1, sector) != 0) {
            return -1;
        }
        if (remaining > 0) {
            src_bytes += copy;
            remaining -= copy;
        }
        cluster = fat[cluster];
        if (cluster == 0xFFFF) {
            break;
        }
    }
    return 0;
}

int fs_ready(void) {
    return g_fs_ready;
}

void fs_iterate_root(void (*cb)(const char *name8, uint16_t start_cluster, void *ctx), void *ctx) {
    if (g_fs_ready == 0 || cb == NULL) {
        return;
    }
    fs_dirent *ents = (fs_dirent *)g_root;
    for (uint32_t i = 0; i < (FS_SECTOR_SIZE / sizeof(fs_dirent)); i++) {
        const char *name = ents[i].name;
        uint16_t start = ents[i].start_cluster;
        int empty = 1;
        for (int j = 0; j < 8; j++) {
            if (name[j] != ' ' && name[j] != 0) {
                empty = 0;
                break;
            }
        }
        if (empty && start == 0) {
            break;
        }
        if (empty || start < 2) {
            continue;
        }
        cb(name, start, ctx);
    }
}
