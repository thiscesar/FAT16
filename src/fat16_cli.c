#include "fat16.h"
#include <stdio.h>

static void menu(void) {
  printf("\n");
  printf("╔══════════════════════════════════════════════╗\n");
  printf("║     MANIPULADOR DE FAT16 (RAIZ)              ║\n");
  printf("╠══════════════════════════════════════════════╣\n");
  printf("║ 1. Listar arquivos                           ║\n");
  printf("║ 2. Mostrar conteúdo de arquivo               ║\n");
  printf("║ 3. Exibir atributos de arquivo               ║\n");
  printf("║ 4. Renomear arquivo                          ║\n");
  printf("║ 5. Remover arquivo                           ║\n");
  printf("║ 6. Inserir novo arquivo                      ║\n");
  printf("║ 0. Sair                                      ║\n");
  printf("╚══════════════════════════════════════════════╝\n");
  printf("Escolha: ");
}

int main(int argc, char *argv[]) {
  Fat16Ctx ctx;
  char path[512];

  printf("FAT16 — MENU\n");

  if (argc > 1) {
    snprintf(path, sizeof(path), "%s", argv[1]);
  } else {
    printf("Caminho da imagem FAT16: ");
    if (scanf("%511s", path) != 1)
      return 1;
  }

  if (!fat16_open(&ctx, path))
    return 1;

  int opt;
  char a[256], b[256], src[512];
  for (;;) { // laço infinito
    menu();
    if (scanf("%d", &opt) != 1)
      break;
    getchar();

    switch (opt) {
    case 1:
      fat16_list_dir(&ctx);
      break;
    case 2:
      printf("Nome do arquivo (8.3): ");
      if (scanf("%255s", a) != 1)
        break;
      fat16_show_file(&ctx, a);
      break;
    case 3:
      printf("Nome do arquivo (8.3): ");
      if (scanf("%255s", a) != 1)
        break;
      fat16_show_attrs(&ctx, a);
      break;
    case 4:
      printf("Nome atual (8.3): ");
      if (scanf("%255s", a) != 1)
        break;
      printf("Novo nome (8.3): ");
      if (scanf("%255s", b) != 1)
        break;
      fat16_rename(&ctx, a, b);
      break;
    case 5:
      printf("Remover qual arquivo (8.3): ");
      if (scanf("%255s", a) != 1)
        break;
      printf("Confirmar remoção de '%s'? (s/n): ", a);
      {
        char c;
        if (scanf(" %c", &c) != 1)
          break;
        if (c == 's' || c == 'S')
          fat16_delete(&ctx, a);
        else
          printf("Cancelado.\n");
      }
      break;
    case 6:
      printf("Caminho do arquivo de origem (host): ");
      if (scanf("%511s", src) != 1)
        break;
      printf("Nome destino na imagem (8.3): ");
      if (scanf("%255s", a) != 1)
        break;
      fat16_create(&ctx, src, a);
      break;
    case 0:
      fat16_close(&ctx);
      return 0;
    default:
      printf("Opção inválida.\n");
    }
  }
  fat16_close(&ctx);
  return 0;
}
