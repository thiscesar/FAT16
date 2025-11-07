# Manipulador FAT16 (diretório raiz)

Implementação para a disciplina de **Sistemas Operacionais** da **UNIVALI** (Curso de **Ciência da Computação**).

## 1. Requisitos

- **GCC**, **Make** e **GDB** (normalmente já presentes no Codespaces ou em distros Linux).
- **VSCode** com extensões recomendadas (o projeto inclui `.vscode/extensions.json`):
  - C/C++ (ms-vscode.cpptools)
  - Clang-Format (xaver.clang-format) — opcional, apenas para formatação

Ubuntu/Debian (local):
```bash
sudo apt update
sudo apt install build-essential gdb clang-format
```

## 2. Estrutura esperada

```
.vscode/
  extensions.json
  launch.json
  settings.json
  tasks.json
imgs/
  disco1.img
  disco2.img
src/
  fat16.h
  fat16_fs.c
  fat16_cli.c
Makefile
```

> As imagens de teste devem ficar em `./imgs`. Evite caminhos absolutos como `/imgs`.

## 3. Abrir no VSCode / Codespaces

- **VSCode**: `File → Open Folder…` e selecione a pasta do projeto.
- **Codespaces**: o repositório abre automaticamente no VSCode Web.

O projeto já traz configurações de build e execução em `.vscode/*`.

## 4. Compilar

### Opção A — Terminal
```bash
make
```
Compila objetos em build/ e gera o binário build/fat16.

### Opção B — VSCode (Task)
- `Terminal → Run Task… → Build (make)` ou `Ctrl+Shift+B`.

## 5. Executar

### Opção A — Terminal
```bash
./build/fat16 ./imgs/disco1.img
# ou
./build/fat16 ./imgs/disco2.img
```

### Opção B — VSCode (Debug/Run)
Abra a aba **Run and Debug** e escolha um dos perfis:
- **Run fat16 (disco1.img)**
- **Run fat16 (disco2.img)**

Os perfis estão em `.vscode/launch.json` e já apontam para `./imgs/…`.

## 6. Uso (menu)

Ao iniciar, o programa exibe um menu:

- `1` listar arquivos do diretório raiz
- `2` exibir conteúdo de um arquivo
- `3` exibir atributos de um arquivo
- `4` renomear arquivo
- `5` remover arquivo
- `6` inserir/criar novo arquivo na imagem (cópia de arquivo do host)
- `0` sair

**Atenção ao nome 8.3**: use formato `NOME.EXT` (até 8 chars + `.` + até 3 chars). A conversão para maiúsculas é automática.

Exemplo (terminal):
```bash
./build/fat16 ./imgs/disco1.img
# opção 1 → lista arquivos
# opção 2 → informe algo como README.TXT
```

## 7. Dicas e problemas comuns

- **Caminho incorreto**: confirme se está usando `./imgs/disco1.img` (com “s”).
  ```bash
  ls -l ./imgs
  ```
- **Permissão inexistente**: as imagens dentro do repo não devem exigir `sudo`. Se copiou de outro lugar, confira:
  ```bash
  ls -l ./imgs/disco1.img
  ```
- **Imagem inválida/corrompida**: o programa reportará erro de Boot/FAT/Root inválidos.
- **Nomes fora do padrão**: lembre-se do formato 8.3 (sem espaços, sem acentos).

## 8. Limpar build
```bash
make clean
```

## 9. Executar com variável (opcional)

Você pode usar `make run` passando uma imagem via variável `IMG`:

```make
IMG ?= ./imgs/disco1.img
TARGET = build/fat16

run: all
	$(TARGET) $(IMG)
```

Uso:
```bash
make run                       # usa ./imgs/disco1.img
make run IMG=./imgs/disco2.img
```

---

Qualquer dúvida, rode `make` e depois `./fat16 ./imgs/disco1.img`. Se aparecer mensagem de erro, verifique o caminho e a estrutura acima.


## Referências

- Microsoft. **FAT: General Overview of On-Disk Format (fatgen103).** Documento clássico que descreve o BPB, diretório raiz (entradas de 32 bytes) e a FAT16/12/32.
  - https://download.microsoft.com/download/9/c/5/9c5b2167-8017-4bae-9fde-d599bac8184a/fatgen103.pdf
- OSDev Wiki. **FAT.** Resumo didático do layout do FAT, entrada 8.3, atributos e encadeamento de clusters.
  - https://wiki.osdev.org/FAT
- Wikipedia. **Design of the FAT file system.** Visão geral e exemplos de FAT16, incluindo estrutura da FAT e diretórios.
  - https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
- rweichler/**FAT16** (GitHub). Implementação de leitura de imagens FAT16 em C; útil como referência de organização e testes.
  - https://github.com/rweichler/FAT16

## Apoio com IA (ChatGPT)

Este trabalho foi inteiramente desenvolvido por mim, Cesar Luiz de Sousa Júnior e minha dupla, Vitor Borges Paes. 
Utilizamos o ChatGPT como **apoio pontual** para:
- estruturar o projeto em arquivos (`fat16.h`, `fat16_fs.c`, `fat16_cli.c`) mantendo a simplicidade;
- revisar e comentar as **structs** (BPB/DirectoryEntry) e constantes, com referências clássicas;
- gerar um **Makefile** e configuração de **VSCode** (tasks/launch/formatador);
- esclarecer detalhes de FAT16 (formato 8.3, cadeia de clusters, datas) e ajustar mensagens de erro;
- criar este README e a seção de referências de maneira organizada.

Exemplos de prompts usados (resumido):
- “Separe meu código em header + implementação + CLI (menu em linha de comando), mantendo tudo simples, sem padrões desnecessários.”
- “Explique `#pragma pack(push, 1)` e comente cada campo do `BootSector` e `DirectoryEntry`.”
- “Crie um Makefile mínimo para `src/fat16_fs.c` e `src/fat16_cli.c`.”
- “Configuração do VSCode (launch/tasks) para rodar `./fat16 ./imgs/disco1.img`.”

Obs.: a autoria do código, decisões de arquitetura e testes são da equipe; o ChatGPT foi utilizado como ferramenta de suporte técnico/redacional.
Decidimos incluir essa seção para manter a transparência com o Professor responsável da disciplina.
