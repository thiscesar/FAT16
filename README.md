# Manipulador FAT16 (diretório raiz)

Implementação didática das operações básicas sobre uma imagem FAT16 (sem subdiretórios).
CLI simples em C usando `gcc`/`make`. Projeto preparado para VSCode e Codespaces.

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
Isso gera o executável `./fat16` na raiz do projeto.

### Opção B — VSCode (Task)
- `Terminal → Run Task… → Build (make)` ou `Ctrl+Shift+B`.

## 5. Executar

### Opção A — Terminal
```bash
./fat16 ./imgs/disco1.img
# ou
./fat16 ./imgs/disco2.img
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
./fat16 ./imgs/disco1.img
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
# adicione ao Makefile
IMG ?= ./imgs/disco1.img

run: all
	./fat16 $(IMG)
```

Uso:
```bash
make run                       # usa ./imgs/disco1.img
make run IMG=./imgs/disco2.img
```

---

Qualquer dúvida, rode `make` e depois `./fat16 ./imgs/disco1.img`. Se aparecer mensagem de erro, verifique o caminho e a estrutura acima.
