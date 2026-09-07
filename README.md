# RayUML Editor

Software para criação de diagramas UML, feito em **C** com
[raylib](https://www.raylib.com/).

## Status

Em desenvolvimento inicial. Já é possível criar classes UML pelo botão de
UI e arrastá-las livremente pelo canvas, com suporte a zoom e pan.

## Funcionalidades

- Canvas infinito com grid, zoom (scroll) e pan (botão direito do mouse).
- Criação de classes UML via botão de UI.
- Arrastar classes pelo canvas.
- Alternar tela cheia (`Alt + Enter`).

## Como compilar e rodar

Veja o guia completo em [docs/BUILD.md](docs/BUILD.md). Resumo (Windows):

```bat
build.bat
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
├── docs/          # Documentação de apoio
├── lib/           # Headers de terceiros (raylib)
├── src/
│   ├── include/    # Headers dos módulos
│   └── modules/    # Implementação dos módulos
├── main.c         # Ponto de entrada
└── build.bat      # Script de build (Windows)
```

Detalhes em [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Licença

Veja [LICENSE](LICENSE).
