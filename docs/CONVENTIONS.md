# Convenções de código

Regras seguidas no projeto para manter consistência. Ao adicionar código
novo, siga o padrão já usado nos módulos existentes (`src/modules/*.c`).

## Nomenclatura

| Elemento                     | Convenção            | Exemplo                          |
|-------------------------------|-----------------------|-----------------------------------|
| Funções (API pública)          | `PascalCase`           | `AddUMLClass`, `DrawWorldGrid`     |
| Structs / Tipos                | `PascalCase`           | `UMLClass`                         |
| Variáveis locais e parâmetros  | `camelCase`            | `worldPos`, `clickCount`           |
| Variáveis globais/estáticas de módulo | `camelCase` com `static` | `static int clickCount = 0;` |
| Constantes/macros              | `SCREAMING_SNAKE_CASE` | `MOUSE_CURSOR_DEFAULT` (da raylib) |
| Arquivos                       | `snake_case` / `lowercase` | `editor.c`, `renderer.h`      |

Isso segue o mesmo padrão de nomenclatura da própria API do raylib
(funções e tipos em `PascalCase`), para manter o código consistente com a
biblioteca usada.

## Organização de arquivos

- Todo módulo novo deve ter um par **header (`src/include/*.h`) +
  implementação (`src/modules/*.c`)**, nunca lógica direto em `main.c`
  além da orquestração do loop principal.
- Headers usam include guards no formato `#ifndef NOME_H` / `#define
  NOME_H` / `#endif` (nome do arquivo em maiúsculas).
- Inclua apenas o que o header realmente precisa; a implementação (`.c`)
  inclui o próprio header primeiro, depois dependências externas.

## Estilo de código

- **C99**, compilado com `-Wall` — trate warnings como algo a corrigir,
  não ignorar.
- Indentação com 4 espaços, sem tabs.
- Chaves de blocos de controle (`if`, `for`, `while`) em nova linha,
  seguindo o estilo já usado no projeto.
- Prefira nomes descritivos a comentários explicando "o quê". Comente
  apenas quando o "porquê" não for óbvio (uma decisão não trivial, uma
  limitação da raylib, um workaround).
- Mensagens de texto voltadas ao usuário final (UI) podem ficar em
  português (ex.: `"Criar Classe"`), já que é o idioma-alvo da aplicação;
  identificadores de código (funções, variáveis, tipos) ficam em inglês.

## Commits

Mensagens de commit seguem o padrão
[Conventional Commits](https://www.conventionalcommits.org/) em português:

```
tipo: descrição curta no imperativo

feat: Adicionando sistema de arrastar classes
fix: Corrigindo posição inicial da classe criada
refactor: Organizando arquivos em pastas
chore: Atualizando configuração do live serve
```

Tipos usados no projeto: `feat`, `fix`, `refactor`, `chore`, `docs`.

## Fluxo de trabalho

A `main` só recebe código pronto. Todo trabalho acontece em uma branch
própria, que entra na `main` por Merge Request no GitLab.

### Nome da branch

`tipo/descricao-em-kebab-case`, usando os mesmos tipos dos commits:

```
feat/salvar-carregar
fix/multiplicidade-sobreposta
refactor/extrair-widgets
docs/fluxo-de-trabalho
```

### Ciclo

```bash
git checkout main && git pull            # parte da main atualizada
git checkout -b feat/minha-feature

# ... commits pequenos e descritivos durante o desenvolvimento ...

git push -u origin feat/minha-feature
glab mr create --fill                    # abre o Merge Request
```

Depois que o MR for aprovado e mesclado:

```bash
git checkout main && git pull
git branch -d feat/minha-feature         # remove a branch local
```

### Escopo de uma branch

Uma branch = uma feature. Se no meio do caminho aparecer algo não
relacionado (um bug em outra área, uma limpeza), vale abrir outra branch a
partir da `main` em vez de misturar — MRs que fazem duas coisas são difíceis
de revisar e de reverter.

### Histórico anterior

Os commits até `6ddfea1` foram feitos direto na `main`, antes desta
convenção. O commit `6ddfea1` em especial junta várias features porque as
mudanças estavam entrelaçadas nos mesmos arquivos; ele não foi dividido
porque já estava publicado, e reescrever histórico publicado exige
`push --force`.
