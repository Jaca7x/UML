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
│   │   ├── history.h         # Desfazer (Ctrl+Z)
│   │   ├── storage.h         # Salvar e carregar o diagrama em arquivo
│   │   ├── renderer.h        # Desenho do mundo (grid)
│   │   ├── codegen.h         # Geração de código Java a partir do diagrama
│   │   ├── codeparse.h       # Leitura de código Java de volta para o diagrama
│   │   ├── validation.h      # Regras de coerência do modelo
│   │   ├── ui.h              # Barra de menu, painel lateral, tela cheia
│   │   ├── uifont.h          # Carregamento e desenho de texto
│   │   └── widgets.h         # Botões e campos do painel de propriedades
│   └── modules/             # Implementação dos módulos (.c), 1:1 com os headers
├── tests/                  # Testes automatizados (make test), rodam sem janela
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

### `history` (`history.h` / `history.c`)

Desfazer por **retrato**: cada passo guarda o diagrama inteiro serializado
pelo `storage`, e `Ctrl+Z` restaura o anterior. A alternativa (guardar a
operação inversa de cada ação) gasta menos memória, mas exige tratar cada
tipo de ação separadamente — mais código e mais chance de erro para um
diagrama deste tamanho.

Reaproveitar a serialização do `storage` evita uma segunda implementação que
poderia divergir da primeira.

`PushHistory()` deve ser chamado **antes** de qualquer alteração. Retratos
idênticos ao passo anterior são descartados, então chamar em excesso não
enche o histórico de passos vazios — é o que permite chamá-lo no início de um
arrasto sem saber ainda se a classe vai se mover.

**Granularidade da digitação:** o retrato é tirado quando um campo de texto
recebe o foco, não a cada tecla. Assim um `Ctrl+Z` desfaz a edição inteira
daquele campo em vez de uma letra por vez.

### `renderer` (`renderer.h` / `renderer.c`)

Desenho de mundo que não é estado do diagrama — hoje, o grid de fundo.

### `codegen` (`codegen.h` / `codegen.c`)

Escreve um `.java` por classe do diagrama dentro da pasta `codigo/`. O objetivo
não é gerar o sistema pronto, e sim o **esqueleto** — as assinaturas que o
diagrama já descreve — para o trabalho começar de um projeto compilável em vez
de um arquivo em branco.

**O tipo da classe vira a palavra-chave:** `class`, `abstract class`,
`interface` ou `enum`. Atributos viram campos, métodos viram assinaturas com
corpo `// TODO implementar`, e a visibilidade UML (`+ - # ~`) vira o modificador
Java (`~` é *package-private*, então não escreve modificador nenhum).

**Tipos são traduzidos, não copiados** (`MapType`): o que se escreve em UML
nem sempre existe em Java — `bool` vira `boolean`, `int` continua `int`. Sem
essa camada o arquivo gerado não compilaria.

**Relacionamentos viram campos.** Agregação e composição com multiplicidade
`*` (ou `0..*`, `1..*`) geram `List<Tipo>` e puxam o `import java.util.List`;
com multiplicidade simples geram um campo do próprio tipo. Herança vira
`extends`, realização vira `implements`.

**Stubs de interface** (`AppendInterfaceStubs`): quem implementa uma interface
precisa dos métodos dela, senão o arquivo gerado não compila. Os que faltam são
emitidos com `@Override` e o mesmo corpo `// TODO`.

**Pacote (opcional).** O campo "pacote java" da aba *Arquivo* faz duas coisas:
escreve a declaração `package` no topo de cada arquivo e espelha o nome na
árvore de pastas — `com.empresa.app` gera `codigo/com/empresa/app/*.java`.
Java exige que a pasta corresponda ao pacote, então gerar tudo plano obrigaria
a mover os arquivos à mão antes de importar num projeto. Campo vazio mantém o
comportamento anterior: arquivos soltos em `codigo/`, sem declaração.

**Limitação conhecida:** gerar de novo **sobrescreve** os arquivos. Código
escrito à mão dentro de `codigo/` se perde — a pasta é saída descartável
(está no `.gitignore`), não lugar de trabalho.

### `codeparse` (`codeparse.h` / `codeparse.c`)

O caminho de volta do `codegen`: lê os `.java` de uma pasta e remonta o
diagrama. É o que fecha o ciclo — gerar, mexer no código, reabrir.

**Não é um parser de Java**, e não tenta ser. É um leitor de linha tolerante,
que reconhece o que o gerador escreve e a forma comum de escrever o resto à
mão. Ficam de fora: declaração quebrada em várias linhas, classe aninhada e
genérico com espaço dentro (`Map<String, Integer>`). Um parser de verdade
custaria muito mais do que entrega para o tamanho deste editor.

**O problema de fundo: Java não guarda coordenada.** E não só isso — agregação
e composição geram exatamente o mesmo código, a multiplicidade some, e
dependência não vira linha nenhuma. Se os arquivos forem o formato de
gravação, cada abertura perderia parte do diagrama.

A saída é o gerador deixar o que falta em marcador de comentário, que o leitor
reconhece na volta. O arquivo continua Java válido:

```java
// @ruml pos <x> <y> <larguraUsuario> <alturaUsuario>
    private List<Cachorro> cachorros; // @ruml rel agregacao Cachorro "1" "0..*"
    // @ruml rel dependencia Pagamento "" ""
```

Com os marcadores o ciclo ida-e-volta é **idêntico** (verificado contra o
`exemplo.ruml`, comparando por nome em vez de id, já que a importação
renumera).

**Java escrito à mão não tem marcador**, e aí entra o palpite: `extends` vira
herança, `implements` vira realização, campo de tipo conhecido vira associação,
`List<Conhecido>` vira agregação `*`, e a posição cai numa grade automática.

**Duas passadas** sobre os arquivos: relacionamento cita classe por nome, então
todas precisam existir antes de qualquer corpo ser lido.

**Método com `@Override` é ignorado.** Ele cumpre contrato herdado, não é membro
próprio da classe — e é justamente o que o `AppendInterfaceStubs` acrescentou
por conta própria na geração. Sem esta regra, cada ida e volta inflaria a
classe com os métodos da interface. Construtor também é ignorado: o modelo do
diagrama não tem onde guardar um.

**Pasta sem `.java` não apaga nada.** A contagem vem antes do
`ClearAllClasses()` — descartar o diagrama e só depois descobrir que não havia
o que ler destruiria o trabalho do usuário por causa de uma pasta vazia.

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

### `validation` (`validation.h` / `validation.c`)

Regras de coerência do modelo, listadas num painel (botão **validar**, aba
*Exibir*).

**Por que passou a existir:** enquanto o diagrama era só um desenho, um erro
nele era um desenho errado. Com a geração de código ele virou entrada de
compilador — uma seta de herança desenhada ao contrário produz
`class Animal extends Cachorro`, em silêncio. As regras existem para o sistema
dizer em voz alta o que antes só aparecia quando alguém tentava compilar o
resultado.

**A divisão é por consequência, não por gravidade sentida:**

| | significa |
|---|---|
| **erro** | o código gerado não compila, ou o arquivo não faz sentido |
| **aviso** | compila, mas quase certamente não é o que se quis dizer |

**Erros:** nome duplicado (duas classes viram o mesmo arquivo, e a segunda
passa por cima da primeira), nome que não é identificador Java válido —
espaço, acento, começar com dígito, ou ser palavra reservada —, atributo sem
tipo (vira `private void x;`), assinatura de método repetida, enum em herança,
interface herdando de classe.

**Avisos:** abstrata herdando de concreta (o formato exato da hierarquia
invertida), multiplicidade em herança ou realização (que não têm "quantos"),
interface com atributo, enum com método (que o gerador ignora), abstrata sem
nenhum método.

**Sobrecarga não é erro.** Mesmo nome com argumentos diferentes é válido em
Java; só a assinatura inteira repetida é que não compila.

**O diagrama limpo tem de ficar calado.** Se as regras acusassem algo no
`exemplo.ruml`, o painel viraria ruído que se aprende a ignorar — e um teste
verifica exatamente isso.

Cada problema guarda o `classId` a que se refere: clicar na linha seleciona a
classe. **Gerar código roda a validação antes** e, havendo erro, a barra de
status diz quantos em vez de anunciar sucesso.

## Testes

`make test` compila os módulos junto com `tests/` e roda tudo **sem abrir
janela**. As funções de arquivo da raylib (`LoadFileText`, `DirectoryExists`,
`LoadDirectoryFiles`) funcionam sem `InitWindow`, e é isso que permite
verificar salvar/carregar e a geração de código no terminal e na CI.

O arnês é próprio, em `tests/tests.h`: trazer um framework de teste para C
custaria mais configuração do que o próprio código testado. São quatro macros
(`CHECK`, `CHECK_INT`, `CHECK_STR`, `CHECK_CONTAINS`/`CHECK_MISSING`), um
contador e um código de saída diferente de zero quando algo falha — que é o
que a CI precisa.

**O que está coberto:** o formato de arquivo (ida e volta, campo entre aspas
vazio, arquivo antigo sem o tipo da classe, extensão completada, detecção de
alteração pendente), a geração de Java (um arquivo por classe, `package` antes
dos `import`, palavra-chave por tipo de classe, relacionamentos virando
código, stubs de interface, tradução de tipo e de visibilidade) e a leitura de
volta (ciclo sem perda, duas voltas estáveis, `@Override` ignorado, Java sem
marcador, pasta vazia não apaga o diagrama) e as regras do modelo (cada uma
dispara no caso que descreve, sobrecarga passa, e o diagrama de referência
fica calado).

**A comparação de diagramas é por nome, nunca por id nem por ordem:** a
importação renumera as classes e as lê em ordem alfabética, então comparar o
`.ruml` cru acusaria diferença onde não há.

**Os testes escrevem nas pastas de saída de verdade** (`build/teste/` e, por um
instante, `codigo/`), e removem o que escreveram pelos nomes do diagrama de
amostra — nunca a pasta inteira, que é onde fica o código do usuário.

**CI** (`.gitlab-ci.yml`): o mesmo `make test` num runner Linux. A raylib é
compilada lá e fica em cache entre execuções; o `Makefile` escolhe compilador
e bibliotecas pelo sistema, então o comando é o mesmo nos dois lugares.

## Roadmap (alto nível)

- [x] Criação, edição e exclusão de classes
- [x] Parâmetros com visibilidade, nome e tipo
- [x] Relacionamentos com multiplicidade
- [x] Painel de propriedades com edição ao vivo
- [x] Salvar e carregar o diagrama
- [x] Desfazer (Ctrl+Z)
- [x] Métodos da classe (terceiro compartimento)
- [x] Seleção múltipla e alinhamento na grade
- [x] Tipos de classe (abstrata, interface, enum)
- [x] Geração de código Java, com pacote opcional
- [x] Importação de código Java de volta para o diagrama
- [ ] Exportar o diagrama como imagem
- [ ] Exportar para PlantUML
- [x] Validação do modelo (painel de problemas)
- [ ] Refazer (Ctrl+Y)
- [x] Testes automatizados e CI (`make test`)

Consulte também [BUILD.md](BUILD.md) e [CONVENTIONS.md](CONVENTIONS.md).
