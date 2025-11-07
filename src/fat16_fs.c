/*
 * fat16_fs.c — Implementação simples da "biblioteca" FAT16 (somente raiz)
 * -----------------------------------------------------------------------
 * Objetivo: manter a lógica de FAT16 isolada da interface/CLI.
 *   - Carrega Boot, FAT e Root;
 *   - Opera apenas no diretório raiz (sem LFN);
 *   - Encapsula estado em Fat16Ctx (sem globais externas);
 *   - Helpers estáticos têm "ligação interna" (visíveis só neste .c).
 */

#include "fat16.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ===== Helpers estáticos: visíveis apenas neste arquivo (.c) ===== */

static int load_boot(Fat16Ctx *ctx) {
    if (fseek(ctx->img, 0, SEEK_SET) != 0) return 0;
    if (fread(&ctx->bpb, sizeof(BootSector), 1, ctx->img) != 1) return 0;
    return 1;
}

static int load_fat(Fat16Ctx *ctx) {
    ctx->fat_size_bytes = (uint32_t)ctx->bpb.fat_size_16 * (uint32_t)ctx->bpb.bytes_per_sector;
    ctx->fat = (uint16_t*)malloc(ctx->fat_size_bytes);
    if (!ctx->fat) return 0;

    long fat0_off = (long)ctx->bpb.reserved_sectors * (long)ctx->bpb.bytes_per_sector;
    if (fseek(ctx->img, fat0_off, SEEK_SET) != 0) return 0;
    if (fread(ctx->fat, ctx->fat_size_bytes, 1, ctx->img) != 1) return 0;

    ctx->fat_entries = ctx->fat_size_bytes / 2;
    return 1;
}

static int save_fat(Fat16Ctx *ctx) {
    long first = (long)ctx->bpb.reserved_sectors * (long)ctx->bpb.bytes_per_sector;
    for (int i = 0; i < ctx->bpb.num_fats; i++) {
        long off = first + (long)i * (long)ctx->fat_size_bytes;
        if (fseek(ctx->img, off, SEEK_SET) != 0) return 0;
        if (fwrite(ctx->fat, ctx->fat_size_bytes, 1, ctx->img) != 1) return 0;
    }
    return 1;
}

static int load_root(Fat16Ctx *ctx) {
    uint32_t bytes_root = (uint32_t)ctx->bpb.root_entry_count * 32u;
    ctx->root_dir_sectors = (bytes_root + ctx->bpb.bytes_per_sector - 1) / ctx->bpb.bytes_per_sector;

    long root_off = (long)(ctx->bpb.reserved_sectors + (ctx->bpb.num_fats * ctx->bpb.fat_size_16))
                  * (long)ctx->bpb.bytes_per_sector;

    ctx->root = (DirectoryEntry*)malloc(ctx->bpb.root_entry_count * sizeof(DirectoryEntry));
    if (!ctx->root) return 0;

    if (fseek(ctx->img, root_off, SEEK_SET) != 0) return 0;
    if (fread(ctx->root, sizeof(DirectoryEntry), ctx->bpb.root_entry_count, ctx->img)
        != ctx->bpb.root_entry_count) return 0;

    ctx->first_data_sector = ctx->bpb.reserved_sectors + (ctx->bpb.num_fats * ctx->bpb.fat_size_16) + ctx->root_dir_sectors;
    return 1;
}

static int save_root(Fat16Ctx *ctx) {
    long root_off = (long)(ctx->bpb.reserved_sectors + (ctx->bpb.num_fats * ctx->bpb.fat_size_16))
                  * (long)ctx->bpb.bytes_per_sector;
    if (fseek(ctx->img, root_off, SEEK_SET) != 0) return 0;
    if (fwrite(ctx->root, sizeof(DirectoryEntry), ctx->bpb.root_entry_count, ctx->img)
        != ctx->bpb.root_entry_count) return 0;
    return 1;
}

static void compute_derived(Fat16Ctx *ctx) {
    ctx->total_sectors = (ctx->bpb.total_sectors_16 != 0) ? ctx->bpb.total_sectors_16 : ctx->bpb.total_sectors_32;
    ctx->cluster_size  = (uint32_t)ctx->bpb.sectors_per_cluster * (uint32_t)ctx->bpb.bytes_per_sector;

    uint32_t non_data = (uint32_t)ctx->bpb.reserved_sectors
                      + (uint32_t)ctx->bpb.num_fats * (uint32_t)ctx->bpb.fat_size_16
                      + (uint32_t)ctx->root_dir_sectors;
    ctx->data_sectors = (ctx->total_sectors > non_data) ? (ctx->total_sectors - non_data) : 0;
    ctx->cluster_count = (ctx->bpb.sectors_per_cluster != 0)
                       ? (ctx->data_sectors / ctx->bpb.sectors_per_cluster) : 0;
}

static void decode_date(uint16_t d, int *day, int *mon, int *year) {
    *day  =  d        & 0x1F;
    *mon  = (d >> 5)  & 0x0F;
    *year = ((d >> 9) & 0x7F) + 1980;
}
static void decode_time(uint16_t t, int *hh, int *mm, int *ss) {
    *ss = (t & 0x1F) * 2;
    *mm = (t >> 5) & 0x3F;
    *hh = (t >> 11) & 0x1F;
}

static uint16_t encode_date(int day, int mon, int year) {
    return (uint16_t)(((year - 1980) << 9) | (mon << 5) | day);
}
static uint16_t encode_time(int hh, int mm, int ss) {
    return (uint16_t)((hh << 11) | (mm << 5) | (ss / 2));
}
static void now_fat(uint16_t *d, uint16_t *t) {
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    *d = encode_date(tmv->tm_mday, tmv->tm_mon + 1, tmv->tm_year + 1900);
    *t = encode_time(tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
}

void fat16_to83(const char *in, char name[8], char ext[3]) {
    memset(name, ' ', 8);
    memset(ext,  ' ', 3);
    const char *dot = strrchr(in, '.');
    if (dot) {
        int nlen = (int)(dot - in); if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = (char)toupper((unsigned char)in[i]);
        int elen = (int)strlen(dot + 1); if (elen > 3) elen = 3;
        for (int i = 0; i < elen; i++) ext[i] = (char)toupper((unsigned char)dot[1 + i]);
    } else {
        int nlen = (int)strlen(in); if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = (char)toupper((unsigned char)in[i]);
    }
}

static void make_readable(const DirectoryEntry *e, char out[13]) {
    int p = 0;
    for (int i = 0; i < 8 && e->filename[i] != ' '; i++) out[p++] = e->filename[i];
    if (e->extension[0] != ' ') {
        out[p++] = '.';
        for (int i = 0; i < 3 && e->extension[i] != ' '; i++) out[p++] = e->extension[i];
    }
    out[p] = '\0';
}

static int entry_free(const DirectoryEntry *e) {
    return (e->filename[0] == 0x00) || ((unsigned char)e->filename[0] == 0xE5);
}
static int entry_regular(const DirectoryEntry *e) {
    if (entry_free(e)) return 0;
    if (e->attributes & ATTR_VOLUME_ID) return 0;
    if (e->attributes & ATTR_DIRECTORY) return 0;
    return 1;
}

static long cluster_offset(Fat16Ctx *ctx, uint16_t cluster) {
    uint32_t sector = ((uint32_t)(cluster - 2) * (uint32_t)ctx->bpb.sectors_per_cluster)
                    + ctx->first_data_sector;
    return (long)sector * (long)ctx->bpb.bytes_per_sector;
}

static DirectoryEntry* find_by_name(Fat16Ctx *ctx, const char *name83) {
    char n[8], x[3]; fat16_to83(name83, n, x);
    for (int i = 0; i < ctx->bpb.root_entry_count; i++) {
        DirectoryEntry *e = &ctx->root[i];
        if (!entry_regular(e)) continue;
        if (memcmp(e->filename, n, 8) == 0 && memcmp(e->extension, x, 3) == 0) return e;
    }
    return NULL;
}

static DirectoryEntry* find_free_dir(Fat16Ctx *ctx) {
    for (int i = 0; i < ctx->bpb.root_entry_count; i++) {
        if (entry_free(&ctx->root[i])) return &ctx->root[i];
    }
    return NULL;
}

static uint16_t find_free_cluster_from(Fat16Ctx *ctx, uint16_t start) {
    if (start < 2) start = 2;
    for (uint32_t i = start; i < ctx->fat_entries; i++) {
        if (ctx->fat[i] == FAT16_FREE) return (uint16_t)i;
    }
    return 0;
}

static uint16_t allocate_chain(Fat16Ctx *ctx, int n, uint16_t *out_chain) {
    uint16_t cursor = 2;
    for (int i = 0; i < n; i++) {
        uint16_t c = find_free_cluster_from(ctx, cursor);
        if (c == 0) {
            for (int k = 0; k < i; k++) ctx->fat[out_chain[k]] = FAT16_FREE;
            return 0;
        }
        ctx->fat[c] = FAT16_EOF;     /* reserva */
        out_chain[i] = c;
        cursor = c + 1;
    }
    for (int i = 0; i < n - 1; i++) ctx->fat[out_chain[i]] = out_chain[i + 1];
    ctx->fat[n - 1] = FAT16_EOF;
    return out_chain[0];
}

static uint8_t* read_chain(Fat16Ctx *ctx, const DirectoryEntry *e, uint32_t *sz) {
    *sz = e->file_size;
    if (*sz == 0) return NULL;
    uint8_t *buf = (uint8_t*)malloc(*sz);
    if (!buf) { printf("Erro de memória.\n"); return NULL; }

    uint32_t got = 0, steps = 0;
    uint16_t c = e->first_cluster_low;
    while (got < *sz) {
        if (c < 2 || c >= ctx->fat_entries) { printf("Cluster fora do limite.\n"); free(buf); return NULL; }
        if (c == FAT16_FREE) { printf("Cadeia interrompida.\n"); free(buf); return NULL; }
        if (c == FAT16_BAD) { printf("Cluster BAD.\n"); free(buf); return NULL; }

        long off = cluster_offset(ctx, c);
        if (fseek(ctx->img, off, SEEK_SET) != 0) { free(buf); return NULL; }

        uint32_t to_read = *sz - got; if (to_read > ctx->cluster_size) to_read = ctx->cluster_size;
        if (fread(buf + got, 1, to_read, ctx->img) != to_read) { printf("Falha leitura.\n"); free(buf); return NULL; }
        got += to_read;

        if (c >= FAT16_EOF_MIN) break;
        uint16_t next = ctx->fat[c];
        if (++steps > ctx->cluster_count + 8) { printf("Loop suspeito.\n"); free(buf); return NULL; }
        c = next;
    }
    return buf;
}

/* ================= Implementação da API pública ================= */

int fat16_open(Fat16Ctx *ctx, const char *img_path) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->img = fopen(img_path, "r+b");
    if (!ctx->img) { printf("Não consegui abrir '%s'.\n", img_path); return 0; }
    if (!load_boot(ctx)) { printf("Boot inválido.\n"); fat16_close(ctx); return 0; }
    if (!load_fat(ctx))  { printf("FAT inválida.\n"); fat16_close(ctx); return 0; }
    if (!load_root(ctx)) { printf("Root inválido.\n"); fat16_close(ctx); return 0; }
    compute_derived(ctx);

    printf("Bytes/Setor=%u  Setores/Cluster=%u  #FATs=%u  RootEntries=%u\n",
           ctx->bpb.bytes_per_sector, ctx->bpb.sectors_per_cluster,
           ctx->bpb.num_fats, ctx->bpb.root_entry_count);
    printf("FAT(setores)=%u  FirstDataSector=%u  ClustersDados=%u\n",
           ctx->bpb.fat_size_16, ctx->first_data_sector, ctx->cluster_count);
    return 1;
}

int fat16_flush(Fat16Ctx *ctx) {
    int ok = 1; ok &= save_fat(ctx); ok &= save_root(ctx); fflush(ctx->img); return ok;
}

void fat16_close(Fat16Ctx *ctx) {
    if (ctx->root) { free(ctx->root); ctx->root = NULL; }
    if (ctx->fat)  { free(ctx->fat);  ctx->fat  = NULL; }
    if (ctx->img)  { fclose(ctx->img); ctx->img = NULL; }
    memset(ctx, 0, sizeof(*ctx));
}

void fat16_list_dir(Fat16Ctx *ctx) {
    (void)ctx;
    printf("\n========== DIRETÓRIO RAIZ ==========\n");
    printf("%-13s %12s\n", "Arquivo", "Tamanho");
    printf("------------------------------------\n");

    int count = 0;
    for (int i = 0; i < ctx->bpb.root_entry_count; i++) {
        DirectoryEntry *e = &ctx->root[i];
        if (!entry_regular(e)) continue;
        char nm[13]; make_readable(e, nm);
        printf("%-13s %12lu\n", nm, (unsigned long)e->file_size);
        count++;
    }
    if (count == 0) printf("(sem arquivos)\n");
    printf("------------------------------------\n");
    printf("Total: %d arquivo(s)\n", count);
}

void fat16_show_file(Fat16Ctx *ctx, const char *name83) {
    DirectoryEntry *e = find_by_name(ctx, name83);
    if (!e) { printf("Arquivo '%s' não encontrado.\n", name83); return; }
    uint32_t sz = 0; uint8_t *data = read_chain(ctx, e, &sz);
    if (!data && sz > 0) return;
    printf("\n--- Conteúdo de '%s' (%lu bytes) ---\n", name83, (unsigned long)sz);
    for (uint32_t i = 0; i < sz; i++) putchar(data ? data[i] : '\0');
    putchar('\n');
    free(data);
}

void fat16_show_attrs(Fat16Ctx *ctx, const char *name83) {
    DirectoryEntry *e = find_by_name(ctx, name83);
    if (!e) { printf("Arquivo '%s' não encontrado.\n", name83); return; }

    int d,m,y,hh,mm,ss;
    printf("\nAtributos de: %s\n", name83);
    printf("Tamanho: %lu bytes\n", (unsigned long)e->file_size);

    decode_date(e->creation_date, &d,&m,&y);
    decode_time(e->creation_time, &hh,&mm,&ss);
    printf("Criado em:   %02d/%02d/%04d %02d:%02d:%02d\n", d,m,y,hh,mm,ss);

    decode_date(e->last_mod_date, &d,&m,&y);
    decode_time(e->last_mod_time, &hh,&mm,&ss);
    printf("Modificado:  %02d/%02d/%04d %02d:%02d:%02d\n", d,m,y,hh,mm,ss);

    printf("Atributos:\n");
    printf("  RO: %s  Hidden: %s  System: %s  Archive: %s\n",
           (e->attributes & ATTR_READ_ONLY) ? "Sim":"Não",
           (e->attributes & ATTR_HIDDEN)    ? "Sim":"Não",
           (e->attributes & ATTR_SYSTEM)    ? "Sim":"Não",
           (e->attributes & ATTR_ARCHIVE)   ? "Sim":"Não");
}

void fat16_rename(Fat16Ctx *ctx, const char *old83, const char *new83) {
    DirectoryEntry *e = find_by_name(ctx, old83);
    if (!e) { printf("Arquivo '%s' não encontrado.\n", old83); return; }
    if (find_by_name(ctx, new83)) { printf("Já existe '%s'.\n", new83); return; }

    char n[8], x[3]; fat16_to83(new83, n, x);
    memcpy(e->filename, n, 8);
    memcpy(e->extension, x, 3);

    uint16_t d,t; now_fat(&d,&t);
    e->last_mod_date = d; e->last_mod_time = t;

    if (!save_root(ctx)) { printf("Erro ao salvar diretório.\n"); return; }
    fflush(ctx->img);
    printf("Renomeado: '%s' -> '%s'\n", old83, new83);
}

void fat16_delete(Fat16Ctx *ctx, const char *name83) {
    DirectoryEntry *e = find_by_name(ctx, name83);
    if (!e) { printf("Arquivo '%s' não encontrado.\n", name83); return; }

    uint16_t c = e->first_cluster_low; uint32_t steps = 0;
    while (c >= 2 && c < ctx->fat_entries && c < FAT16_EOF_MIN) {
        uint16_t nx = ctx->fat[c];
        ctx->fat[c] = FAT16_FREE;
        c = nx;
        if (++steps > ctx->cluster_count + 8) { printf("Loop suspeito.\n"); break; }
    }
    e->filename[0] = (char)0xE5; /* marca deletado */

    if (!save_fat(ctx) || !save_root(ctx)) { printf("Erro ao salvar.\n"); return; }
    fflush(ctx->img);
    printf("Removido: '%s'\n", name83);
}

void fat16_create(Fat16Ctx *ctx, const char *host_src, const char *dest83) {
    FILE *src = fopen(host_src, "rb");
    if (!src) { printf("Não abri '%s'.\n", host_src); return; }

    if (find_by_name(ctx, dest83)) { printf("Já existe '%s'.\n", dest83); fclose(src); return; }
    DirectoryEntry *slot = find_free_dir(ctx);
    if (!slot) { printf("Diretório raiz cheio.\n"); fclose(src); return; }

    if (fseek(src, 0, SEEK_END) != 0) { fclose(src); return; }
    long fsz = ftell(src); if (fsz < 0) { fclose(src); return; }
    if (fseek(src, 0, SEEK_SET) != 0) { fclose(src); return; }

    int need = (int)((fsz + (long)ctx->cluster_size - 1) / (long)ctx->cluster_size);
    if (need <= 0) need = 1;

    uint16_t *chain = (uint16_t*)malloc(sizeof(uint16_t) * (size_t)need);
    if (!chain) { printf("Memória insuficiente.\n"); fclose(src); return; }

    uint16_t first = allocate_chain(ctx, need, chain);
    if (first == 0) { printf("Sem clusters livres suficientes.\n"); free(chain); fclose(src); return; }

    uint8_t *buf = (uint8_t*)malloc(ctx->cluster_size);
    if (!buf) { printf("Memória insuficiente.\n"); free(chain); fclose(src); return; }

    for (int i = 0; i < need; i++) {
        size_t to_read = ctx->cluster_size;
        if (i == need - 1) {
            long rem = fsz - (long)i * (long)ctx->cluster_size;
            if (rem < 0) rem = 0;
            if ((size_t)rem < to_read) to_read = (size_t)rem;
        }
        size_t got = fread(buf, 1, to_read, src);
        if (got < to_read) memset(buf + got, 0, ctx->cluster_size - got);

        long off = cluster_offset(ctx, chain[i]);
        fseek(ctx->img, off, SEEK_SET);
        fwrite(buf, 1, ctx->cluster_size, ctx->img);
    }

    free(buf);
    fclose(src);

    memset(slot, 0, sizeof(*slot));
    char n[8], x[3]; fat16_to83(dest83, n, x);
    memcpy(slot->filename, n, 8);
    memcpy(slot->extension, x, 3);
    slot->attributes = ATTR_ARCHIVE;
    slot->first_cluster_low = first;
    slot->file_size = (uint32_t)fsz;

    uint16_t d,t; now_fat(&d,&t);
    slot->creation_date = d; slot->creation_time = t;
    slot->last_mod_date = d; slot->last_mod_time = t;
    slot->last_access_date = d;

    if (!save_fat(ctx) || !save_root(ctx)) { printf("Erro ao gravar metadados.\n"); free(chain); return; }
    fflush(ctx->img);
    free(chain);
    printf("Criado '%s' (%ld bytes).\n", dest83, fsz);
}
