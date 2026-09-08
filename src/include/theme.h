#ifndef THEME_H
#define THEME_H
#include "../../lib/raylib.h"

// Paleta central da interface. Nenhum modulo deve usar cor fixa da raylib:
// e o que permite trocar entre claro e escuro sem caçar valor espalhado.

void ToggleTheme(void);
bool IsDarkMode(void);

Color ThemeCanvas(void);      // fundo do canvas
Color ThemeGrid(void);        // linhas da grade
Color ThemeMenuBar(void);     // barra do topo
Color ThemePanel(void);       // painel lateral
Color ThemeSurface(void);     // fundo de botao, campo e caixa de classe
Color ThemeBorder(void);      // contorno neutro
Color ThemeText(void);        // texto principal
Color ThemeTextMuted(void);   // rotulos e texto secundario
Color ThemeAccent(void);      // selecao e destaque
Color ThemeDanger(void);      // erro e exclusao
Color ThemeSuccess(void);     // confirmacao

#endif
