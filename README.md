# RayUML Editor

Software para criação de diagramas UML, feito em **C** com
[raylib](https://www.raylib.com/).

## Status

Em desenvolvimento. Já é possível montar um diagrama de classes completo —
classes com atributos tipados e relacionamentos com multiplicidade — e salvar
o resultado em arquivo.

## Funcionalidades

- Canvas infinito com grid, zoom (scroll) e pan (botão direito do mouse).
- Classes com nome e lista de atributos (visibilidade, nome e tipo), com a
  caixa se ajustando ao conteúdo e alças para redimensionar manualmente.
- Os 6 relacionamentos do UML (associação, herança, agregação, composição,
  dependência e realização) com multiplicidade nas duas pontas. As linhas
  ancoram nas bordas e acompanham as classes ao arrastar.
- Painel de propriedades lateral com edição ao vivo.
- Salvar e carregar em `diagrama.ruml` (formato de texto legível). Para abrir
  outro arquivo, arraste-o para dentro da janela.
- Tela cheia pelo botão ou `Alt + Enter`.

## Como compilar e rodar

Veja o guia completo em [docs/BUILD.md](docs/BUILD.md). Resumo (Windows):

```bat
make run
```

## Documentação

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — estrutura de pastas,
  módulos e fluxo de execução.
- [docs/BUILD.md](docs/BUILD.md) — como compilar/rodar (desktop e Android).
- [docs/CONVENTIONS.md](docs/CONVENTIONS.md) — convenções de nomenclatura,
  estilo de código e commits.

## Estrutura do projeto

```
UML/
├── assets/fonts/  # Fonte opcional da interface (ver BUILD.md)
├── docs/          # Documentação de apoio
├── lib/           # Headers de terceiros (raylib)
├── src/
│   ├── include/    # Headers dos módulos
│   └── modules/    # editor, relations, ui, widgets, uifont, renderer
├── main.c         # Ponto de entrada
└── Makefile       # Build incremental
```

Detalhes em [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Licença

Veja [LICENSE](LICENSE).
