#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <stdio.h>

/* ======== CONSTANTES/FLAGS BÁSICAS DO FAT16 (públicas) ======== */
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

#define FAT16_FREE 0x0000
#define FAT16_BAD 0xFFF7
#define FAT16_EOF_MIN 0xFFF8
#define FAT16_EOF 0xFFFF

#pragma pack(push, 1)
typedef struct {
  uint8_t jmp_boot[3];
  char oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t num_fats;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t fat_size_16;
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
} BootSector;

typedef struct {
  char filename[8];
  char extension[3];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_time_tenth;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access_date;
  uint16_t first_cluster_high;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint16_t first_cluster_low;
  uint32_t file_size;
} DirectoryEntry;
#pragma pack(pop)

typedef struct {
  /* recursos principais */
  FILE *img;
  BootSector bpb;
  DirectoryEntry *root;
  uint16_t *fat;

  /* derivados úteis */
  uint32_t total_sectors;
  uint32_t fat_size_bytes;
  uint32_t fat_entries;
  uint32_t cluster_size;
  uint32_t root_dir_sectors;
  uint32_t first_data_sector;
  uint32_t data_sectors;
  uint32_t cluster_count;
} Fat16Ctx;

/* ======== API PÚBLICA ======== */

/* Abre e carrega uma imagem FAT16 (somente raiz). Retorna 0 em erro. */
int fat16_open(Fat16Ctx *ctx, const char *img_path);

/* Salva FAT e root de volta (em geral operações já salvam). */
int fat16_flush(Fat16Ctx *ctx);

/* Fecha e libera tudo. */
void fat16_close(Fat16Ctx *ctx);

/* Operações solicitadas no trabalho: */
void fat16_list_dir(Fat16Ctx *ctx);
void fat16_show_file(Fat16Ctx *ctx, const char *name83);
void fat16_show_attrs(Fat16Ctx *ctx, const char *name83);
void fat16_rename(Fat16Ctx *ctx, const char *old83, const char *new83);
void fat16_delete(Fat16Ctx *ctx, const char *name83);
void fat16_create(Fat16Ctx *ctx, const char *host_src_path, const char *dest83);

/* Utilitário (opcionalmente útil fora): converte para nome 8.3 em CAIXA ALTA */
void fat16_to83(const char *in, char name[8], char ext[3]);

#endif /* FAT16_H */
