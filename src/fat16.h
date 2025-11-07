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

/*
 * BootSector (BPB – BIOS Parameter Block)
 * --------------------------------------
 * Representa o setor de boot do volume. Fornece parâmetros físicos/lógicos
 * necessários para localizar FAT, diretório raiz e área de dados.
 * Campos principais usados no código:
 *  - bytes_per_sector: tamanho do setor físico (tipicamente 512)
 *  - sectors_per_cluster: setores por cluster (define o tamanho do cluster)
 *  - reserved_sectors: setores reservados antes da FAT (normalmente 1: o
 * próprio boot)
 *  - num_fats: número de cópias da FAT (geralmente 2)
 *  - root_entry_count: número fixo de entradas no diretório raiz (FAT16)
 *  - total_sectors_16/32: total de setores do volume (usar 32 se 16==0)
 *  - fat_size_16: tamanho de cada FAT em setores
 * Demais campos são metadados de mídia/boot e não afetam as operações básicas.
 */
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

/*
 * DirectoryEntry (entrada de diretório raiz)
 * ------------------------------------------
 * Estrutura de 32 bytes que descreve cada arquivo no diretório raiz.
 * - filename[8] + extension[3]: nome 8.3 em CAIXA ALTA, preenchido com espaços.
 *   * filename[0] == 0x00 → entrada nunca usada (ou fim da lista)
 *   * filename[0] == 0xE5 → entrada marcada como apagada
 * - attributes: flags ATTR_* (read-only, hidden, system, volume label,
 * directory, archive)
 * - creation_time/_date, last_mod_time/_date, last_access_date: carimbos de
 * data/hora (formato FAT)
 * - first_cluster_low: primeiro cluster do conteúdo do arquivo (em FAT16 o
 * “high” costuma ser 0)
 * - file_size: tamanho do arquivo em bytes
 */
typedef struct {
  char filename[8];
  char extension[3];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_time_tenth;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access_date;
  uint16_t
      first_cluster_high; /* em FAT16 normalmente 0; campo é usado em FAT32 */
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint16_t first_cluster_low; /* cluster inicial do arquivo na área de dados */
  uint32_t file_size;         /* tamanho em bytes do arquivo */
} DirectoryEntry;

#pragma pack(pop)

/*
 * Fat16Ctx (contexto de trabalho)
 * -------------------------------
 * Estado mantido em memória após abrir a imagem:
 * - img: FILE* para o arquivo .img (aberto em r+b)
 * - bpb: BootSector lido do setor 0
 * - root: vetor com todas as entradas do diretório raiz (root_entry_count)
 * - fat: cópia da FAT em RAM (cada entrada 16 bits)
 * - campos derivados: tamanhos/offsets já pré-calculados para simplificar
 * operações
 */
typedef struct {
  /* recursos principais */
  FILE *img;
  BootSector bpb;
  DirectoryEntry *root;
  uint16_t *fat;

  /* derivados úteis (pré-calculados a partir do BPB) */
  uint32_t total_sectors;
  uint32_t fat_size_bytes;
  uint32_t fat_entries;
  uint32_t cluster_size;      /* bytes_per_sector * sectors_per_cluster */
  uint32_t root_dir_sectors;  /* setores ocupados pelo diretório raiz */
  uint32_t first_data_sector; /* onde começa a área de dados (cluster 2) */
  uint32_t data_sectors;
  uint32_t cluster_count; /* quantidade total de clusters de dados */
} Fat16Ctx;

/* ======== API PÚBLICA ======== */

/* Abre e carrega uma imagem FAT16 (somente raiz). Retorna 0 em erro. */
int fat16_open(Fat16Ctx *ctx, const char *img_path);

/* Salva FAT e root de volta (em geral operações já salvam). */
int fat16_flush(Fat16Ctx *ctx);

/* Fecha e libera tudo. */
void fat16_close(Fat16Ctx *ctx);

/* Operações solicitadas no enunciado (válidas apenas no diretório raiz): */
void fat16_list_dir(Fat16Ctx *ctx);
void fat16_show_file(Fat16Ctx *ctx, const char *name83);
void fat16_show_attrs(Fat16Ctx *ctx, const char *name83);
void fat16_rename(Fat16Ctx *ctx, const char *old83, const char *new83);
void fat16_delete(Fat16Ctx *ctx, const char *name83);
void fat16_create(Fat16Ctx *ctx, const char *host_src_path, const char *dest83);

/* Converte um nome "livre" para formato 8.3 (CAIXA ALTA, preenchido com
 * espaços). */
void fat16_to83(const char *in, char name[8], char ext[3]);

#endif /* FAT16_H */
