# Build — RayUML Editor

## Pré-requisitos (Windows / Desktop)

- [w64devkit](https://github.com/skeeto/w64devkit) (ou outro toolchain
  GCC/MinGW compatível) instalado em `C:\raylib\w64devkit`.
- [raylib](https://www.raylib.com/) compilada em `C:\raylib\raylib`
  (o script espera `raylib/src` com `libraylib.a`/headers).

Esses caminhos são fixos no `build.bat` (`PATH` e `RAYLIB_PATH`). Se o seu
ambiente usa outra localização, ajuste as variáveis no topo do script.

## Compilar e rodar (Desktop)

Na raiz do projeto:

```bat
build.bat
```

O script:

1. Encerra qualquer `game.exe` em execução.
2. Compila `main.c` + todos os `.c` de `src/modules/` com GCC
   (`-std=c99`, `-DPLATFORM_DESKTOP`).
3. Linka com raylib e as bibliotecas do Windows necessárias (`opengl32`,
   `gdi32`, `winmm`, `shell32`, `user32`).
4. Se a compilação for bem-sucedida, executa `game.exe` automaticamente.

Erros de compilação são exibidos no terminal; o executável só roda se o
build passar (`errorlevel == 0`).

### Alternativa via VS Code

`.vscode/tasks.json` define tasks `build debug` e `build release` usando
`make`/`mingw32-make`, seguindo o padrão de projetos raylib
(`RAYLIB_PATH=C:/raylib/raylib`). Use `Ctrl+Shift+B` no VS Code.

## Compilar para Android

`Makefile.Android` é o Makefile padrão de projetos raylib para Android
(build de APK). Requer Android SDK, NDK/toolchain standalone e JDK
configurados nas variáveis do topo do arquivo (`ANDROID_HOME`,
`ANDROID_TOOLCHAIN`, `JAVA_HOME`, etc.) — ajuste conforme o seu ambiente
antes de rodar `make -f Makefile.Android`.

## Live reload durante desenvolvimento

O fluxo de desenvolvimento usa `nodemon` (não versionado no repositório)
para observar os arquivos-fonte e disparar `build.bat` automaticamente a
cada alteração, evitando compilar/rodar manualmente a cada mudança. Exemplo
de uso (requer Node.js instalado):

```bat
nodemon --exec build.bat --watch main.c --watch src -e c,h
```

Ajuste os parâmetros conforme a sua configuração local.
