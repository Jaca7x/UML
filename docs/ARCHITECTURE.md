# Arquitetura — RayUML Editor

Documento de apoio para entender como o projeto está organizado. Mantenha-o
atualizado sempre que a estrutura de módulos mudar.

## Visão geral

RayUML Editor é uma aplicação desktop (C + [raylib](https://www.raylib.com/))
para criação visual de diagramas UML: um canvas com câmera 2D (pan/zoom) onde
o usuário cria classes, edita seus parâmetros e liga umas às outras com os
relacionamentos do UML.

A interface é dividida em duas regiões: o **canvas** (espaço de mundo, sujeito
a pan/zoom) e a **UI de tela** — a barra de menu no topo e o painel de
propriedades à direita, que não são afetados pela câmera.

## Estrutura de pastas

```
UML/
├── assets/
│   └── fonts/              # ui.ttf opcional (ver BUILD.md)
├── docs/                   # Documentação de apoio (este arquivo, build, convenções)
├── lib/                    # Headers de terceiros (raylib.h, raymath.h)
├── src/
│   ├── include/             # Headers públicos dos módulos (.h)
│   │   ├── editor.h          # UMLClass, UMLParam e API das classes
│   │   ├── relations.h       # UMLRelation, RelationType e API dos relacionamentos
│   │   ├── storage.h         # Salvar e carregar o diagrama em arquivo
│   │   ├── renderer.h        # Desenho do mundo (grid)
│   │   ├── ui.h              # Barra de menu, painel lateral, tela cheia
│   │   ├── uifont.h          # Carregamento e desenho de texto
│   │   └── widgets.h         # Botões e campos do painel de propriedades
│   └── modules/             # Implementação dos módulos (.c), 1:1 com os headers
├── main.c                  # Ponto de entrada: janela, câmera, game loop
├── Makefile                # Build incremental para desktop (recomendado, ver BUILD.md)
├── build.bat               # Script de build alternativo para Windows (não incremental)
├── Makefile.Android        # Build para Android (raylib build system)
└── .vscode/                # Tasks e configs do editor (build debug/release via make)
```

Cada módulo segue o padrão **1 header em `src/include/` + 1 implementação em
`src/modules/`**, com include guards (`#ifndef ... #define ... #endif`). Novos
módulos devem seguir o mesmo par header/implementação.

## Módulos

### `editor` (`editor.h` / `editor.c`)

Dono do estado do diagrama: o array dinâmico de `UMLClass`, a seleção atual e
a lógica de criar, arrastar, redimensionar e editar classes.

- `UMLClass`: posição/tamanho (`bounds`), identidade (`id`, `name`), lista de
  `UMLParam` (visibilidade, nome, tipo), tamanho manual (`userWidth`,
  `userHeight`) e estado de arrasto.
- `UpdateAndDrawBoxes(camera, cursor)`: input e desenho das classes, em espaço
  de mundo.
- `DrawClassProperties(area, cursor)`: conteúdo do painel lateral quando há uma
  classe selecionada.
- Acessores (`GetClassBounds`, `FindClassIndexById`, `GetClassIndexAt`, ...)
  usados pelo módulo de relacionamentos.

**Ids estáveis:** cada classe recebe um `id` único na criação, que nunca muda.
Relacionamentos guardam esse id, não o índice do array — excluir uma classe
reordena o array, o que quebraria as ligações.

**Tamanho da caixa:** o conteúdo define o **mínimo**. `userWidth`/`userHeight`
guardam o tamanho definido nas alças de redimensionamento e só valem quando
maiores que o conteúdo (`0` = automático).

### `relations` (`relations.h` / `relations.c`)

Dono dos relacionamentos: os 6 tipos do UML (associação, herança, agregação,
composição, dependência, realização), multiplicidade nas duas pontas, e todo
o desenho de linhas e ornamentos.

- Criação por dois caminhos: o botão "Relacionar" (dois cliques) ou as setas
  que aparecem ao redor da classe (arrastar ou clicar).
- `DrawRelationProperties(area, cursor)`: painel lateral do relacionamento.
- Valida duplicata exata e **herança circular** (percorrendo a cadeia inteira,
  não só o caso direto).
- Relacionamentos cujas classes sumiram são removidos sozinhos a cada frame.

**Âncoras nas bordas:** as linhas não saem do centro das caixas. Cada ponta
escolhe a borda pela posição relativa e, quando várias conexões saem da mesma
borda, são distribuídas ao longo dela para não se sobrepor. Isso é
pré-calculado uma vez por frame em `RebuildGeometry()`, agrupando por
`(classe, lado)` — ponta a ponta seria `O(r² × c)`.

### `ui` (`ui.h` / `ui.c`)

Barra de menu do topo (Criar Classe, Relacionar, Tela Cheia) e o painel de
propriedades à direita. O painel desenha a moldura e delega o conteúdo para
`DrawClassProperties` ou `DrawRelationProperties`, conforme a seleção.

`IsMouseOverUi()` cobre as duas regiões e é o que impede o canvas de reagir a
cliques que pertencem à interface.

### `widgets` (`widgets.h` / `widgets.c`)

Botões, campos de texto e rótulos usados pelos painéis. Existe para os módulos
`editor` e `relations` não duplicarem esses controles.

### `uifont` (`uifont.h` / `uifont.c`)

Carrega a fonte e desenha todo o texto do programa. Procura, nesta ordem,
`assets/fonts/ui.ttf` e a fonte do sistema; sem nenhuma, usa a fonte padrão da
raylib.

**Por que existe:** a fonte padrão da raylib é um bitmap de 10px e fica
irregular em qualquer tamanho que não seja múltiplo dela. O módulo carrega
**um atlas por tamanho usado** (12, 14, 16) e desenha sempre no tamanho do
atlas, mantendo a escala em 1:1 — escalar a textura é o que borra o texto.

### `storage` (`storage.h` / `storage.c`)

Grava e lê o diagrama em um formato de texto próprio, uma linha por elemento:

```
# RayUML 1
class <id> <x> <y> <userWidth> <userHeight> "<nome>"
param <visibilidade> "<nome>" "<tipo>"
relation <origem> <destino> <tipo> "<mult origem>" "<mult destino>"
```

Os `param` pertencem sempre à última `class` lida. O que o usuário digita vai
entre aspas, porque nome e tipo aceitam espaço.

**Por que formato próprio e não PlantUML:** PlantUML descreve a estrutura mas
não guarda coordenadas nem tamanho das caixas. Salvar nele perderia todo o
layout ao reabrir. PlantUML continua bom como exportação — só não serve para
salvar.

**Tipo do relacionamento é gravado por nome** (`composicao`, `heranca`, ...),
não pelo valor numérico do enum: reordenar os tipos invalidaria os arquivos
já salvos.

### `renderer` (`renderer.h` / `renderer.c`)

Desenho de mundo que não é estado do diagrama — hoje, o grid de fundo.

## Fluxo de execução (`main.c`)

1. Declara suporte a DPI (`FLAG_WINDOW_HIGHDPI`), cria a janela, carrega a
   fonte e inicializa a `Camera2D`.
2. Loop principal:
   - Zoom (scroll), pan (botão direito) e atalho de tela cheia.
   - Dentro de `BeginMode2D`/`EndMode2D`: grid → **relacionamentos** →
     classes. As linhas vêm antes para ficarem atrás das caixas.
   - Fora do `Mode2D`: barra de menu e painel lateral.
   - Aplica o cursor do frame.

**Ordem importa.** `UpdateAndDrawRelations` roda antes de `UpdateAndDrawBoxes`,
então o módulo de relacionamentos processa o clique primeiro. Quando ele
consome um clique (criar ou selecionar um relacionamento), sinaliza via
`DidRelationsConsumeClick()` e o editor ignora aquele frame — sem isso o mesmo
clique selecionaria também a classe embaixo do cursor.

**Cursor:** o ponteiro `int *cursor` circula entre `main` e os módulos para que
cada um "peça" um cursor (mão sobre botão, seta diagonal na alça) sem chamar
`SetMouseCursor` direto, mantendo a decisão final no loop principal.

## Roadmap (alto nível)

- [x] Criação, edição e exclusão de classes
- [x] Parâmetros com visibilidade, nome e tipo
- [x] Relacionamentos com multiplicidade
- [x] Painel de propriedades com edição ao vivo
- [x] Salvar e carregar o diagrama
- [ ] Exportar o diagrama como imagem
- [ ] Exportar para PlantUML
- [ ] Métodos da classe (terceiro compartimento; o campo `methods` já existe
      na struct mas não é usado)
- [ ] Desfazer/refazer
- [ ] Seleção múltipla e alinhamento na grade

Consulte também [BUILD.md](BUILD.md) e [CONVENTIONS.md](CONVENTIONS.md).
