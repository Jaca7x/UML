# Arquitetura — RayUML Editor

Documento de apoio para entender como o projeto está organizado. Mantenha-o
atualizado sempre que a estrutura de módulos mudar.

## Visão geral

RayUML Editor é uma aplicação desktop (C + [raylib](https://www.raylib.com/))
para criação visual de diagramas UML: um canvas com câmera 2D (pan/zoom) onde
o usuário cria e arrasta "classes" representadas como caixas.

## Estrutura de pastas

```
UML/
├── docs/                 # Documentação de apoio (este arquivo, build, convenções)
├── lib/                  # Headers de terceiros (raylib.h, raymath.h)
├── src/
│   ├── include/           # Headers públicos dos módulos (.h)
│   │   ├── editor.h        # Struct UMLClass e API de criação/atualização de classes
│   │   ├── renderer.h      # API de desenho do mundo (grid, etc.)
│   │   └── ui.h             # API dos elementos de interface (botões, painéis)
│   └── modules/            # Implementação dos módulos (.c), 1:1 com os headers
│       ├── editor.c
│       ├── renderer.c
│       └── ui.c
├── main.c                 # Ponto de entrada: janela, câmera, game loop
├── build.bat               # Script de build para Windows (desktop, MinGW/w64devkit)
├── Makefile.Android         # Build para Android (raylib build system)
└── .vscode/                 # Tasks e configs do editor (build debug/release via make)
```

Cada módulo segue o padrão **1 header em `src/include/` + 1 implementação em
`src/modules/`**, com include guards (`#ifndef ... #define ... #endif`). Novos
módulos devem seguir o mesmo par header/implementação.

## Módulos

### `editor` (`src/include/editor.h`, `src/modules/editor.c`)

Dono do estado do diagrama: o array dinâmico de `UMLClass` e a lógica de
criar, selecionar e arrastar classes.

- `UMLClass`: struct com posição/tamanho (`bounds`), identidade (`id`,
  `name`), conteúdo (`atributes`, `methods`) e estado de interação
  (`isSelected`, `isDragging`, `dragOffSet`).
- `AddUMLClass(Camera2D camera)`: cria uma nova classe.
- `UpdateAndDrawBoxes(Camera2D camera, int *cursor)`: atualiza input
  (seleção/drag) e desenha todas as classes a cada frame. Recebe um ponteiro
  para o cursor do frame (`cursor`) para que módulos de UI/editor possam
  sinalizar qual cursor deve ser exibido, sem cada módulo chamar
  `SetMouseCursor` diretamente.

### `renderer` (`src/include/renderer.h`, `src/modules/renderer.c`)

Responsável por desenho de "mundo" que não é estado do diagrama em si —
hoje, o grid de fundo (`DrawWorldGrid`).

### `ui` (`src/include/ui.h`, `src/modules/ui.c`)

Elementos de interface que vivem em espaço de tela (não afetados pela
câmera), como o botão "Criar Classe" (`DrawUi`). Chama para dentro do
`editor` (ex.: `AddUMLClass`) quando o usuário interage com um botão.

## Fluxo de execução (`main.c`)

1. Cria a janela e a `Camera2D` (zoom inicial 1.0, offset no centro da tela).
2. Loop principal (`while (!WindowShouldClose())`):
   - Lê input de zoom (scroll) e pan (botão direito do mouse) e atualiza a
     câmera.
   - Trata atalho de fullscreen (`Alt+Enter`).
   - Desenha, em ordem: grid do mundo → classes UML (dentro de
     `BeginMode2D`/`EndMode2D`, ou seja, sujeitos à câmera) → UI de tela
     (fora do `Mode2D`, em coordenadas de tela).
   - Aplica o cursor do frame (`SetMouseCursor`) definido pelos módulos.

O ponteiro `int *cursor` que circula entre `main`, `editor` e `ui` é o
mecanismo usado para os módulos "pedirem" um cursor específico (ex.: mão ao
passar sobre um botão, resize ao arrastar) sem cada um chamar a API do
raylib diretamente — mantém a decisão final centralizada no loop principal.

## Roadmap (alto nível)

- [ ] Persistir/serializar o diagrama (salvar/carregar `.json` ou formato
      próprio).
- [ ] Edição de atributos/métodos das classes (hoje os campos existem na
      struct mas não são editáveis pela UI).
- [ ] Relacionamentos entre classes (herança, associação, composição, etc.).
- [ ] Seleção múltipla / exclusão de classes.

Consulte também [BUILD.md](BUILD.md) e [CONVENTIONS.md](CONVENTIONS.md).
