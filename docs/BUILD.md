# Build — RayUML Editor

## Pré-requisitos (Windows / Desktop)

- [w64devkit](https://github.com/skeeto/w64devkit) (ou outro toolchain
  GCC/MinGW compatível) instalado em `C:\raylib\w64devkit`.
- [raylib](https://www.raylib.com/) compilada em `C:\raylib\raylib`
  (o `Makefile`/`build.bat` esperam `raylib/src` com `libraylib.a`/headers).

Esses caminhos são os padrões usados pelo `Makefile` e pelo `build.bat`. Se
o seu ambiente usa outra localização, sobrescreva via variável
(`RAYLIB_PATH=...`) ou ajuste as variáveis no topo dos arquivos.

## Compilar e rodar — `make` (recomendado)

Na raiz do projeto, com `C:\raylib\w64devkit\bin` disponível (o `Makefile`
já usa o gcc de lá diretamente, não precisa estar no `PATH`):

```bat
make               :: build release (game.exe)
make BUILD_MODE=DEBUG   :: build debug (símbolos, sem otimização)
make run           :: build + executa
make clean         :: remove build/ e game.exe
```

Diferente do `build.bat`, o `Makefile` só recompila os `.c` que mudaram
(ou cujos headers mudaram) desde o último build — build incremental, bem
mais rápido a cada alteração pequena.

### Build + debug pelo VS Code (`F5`)

`.vscode/tasks.json` define as tasks `build debug` e `build release`
(chamam o `Makefile` acima) e `.vscode/launch.json` usa essas tasks como
`preLaunchTask`. Ou seja, `F5` no VS Code já compila e abre o jogo com o
`gdb` anexado (breakpoints, step, inspeção de variáveis), sem precisar de
terminal nem de watcher externo.

- **Debug** (`F5` padrão): build debug + gdb anexado.
- **Run**: build release + executa.

## Compilar e rodar — `build.bat` (alternativa simples)

```bat
build.bat
```

Faz um build completo (não incremental — recompila tudo sempre) e roda
`game.exe` em seguida. Útil como um comando único e "burro" para rodar por
fora do VS Code, ou como alvo de um watcher (veja abaixo).

## Fonte da interface (opcional)

A interface tenta carregar `assets/fonts/ui.ttf` na inicializacao. Se o
arquivo nao existir, cai automaticamente na fonte padrao da raylib — o
projeto compila e roda normalmente sem ele.

A fonte padrao e um bitmap de 10px, entao qualquer tamanho que nao seja
multiplo de 10 sai com escala quebrada e aspecto irregular. Para a
interface ficar nitida, coloque um `.ttf` de licenca livre nesse caminho:

```
assets/fonts/ui.ttf
```

Boas opcoes (todas com licenca aberta): [Inter](https://rsms.me/inter/),
[Roboto](https://fonts.google.com/specimen/Roboto) ou
[DejaVu Sans](https://dejavu-fonts.github.io/). Basta renomear o arquivo
para `ui.ttf`.

## Compilar para Android

`Makefile.Android` é o Makefile padrão de projetos raylib para Android
(build de APK). Requer Android SDK, NDK/toolchain standalone e JDK
configurados nas variáveis do topo do arquivo (`ANDROID_HOME`,
`ANDROID_TOOLCHAIN`, `JAVA_HOME`, etc.) — ajuste conforme o seu ambiente

## Testes

```
make test
```

Compila os módulos junto com `tests/` e roda a suíte **sem abrir janela**.
Sai com código diferente de zero se algo falhar, que é o que a CI usa.

Os testes escrevem em `build/teste/` e, por um instante, em `codigo/` —
removendo depois só os arquivos de amostra que criaram. Se você tinha código
gerado solto em `codigo/`, basta gerar de novo.

Detalhes do que está coberto e por quê: [ARCHITECTURE.md](ARCHITECTURE.md).
antes de rodar `make -f Makefile.Android`.

## Live reload durante desenvolvimento (opcional)

Para recompilar/relançar automaticamente ao salvar um arquivo (sem
precisar apertar `F5` a cada mudança), use um watcher de arquivos chamando
`make run` — por exemplo `watchexec` (binário único, sem depender de
Node.js):

```bat
watchexec -w main.c -w src -e c,h -- make run
```

Ajuste os parâmetros conforme a sua configuração local.
