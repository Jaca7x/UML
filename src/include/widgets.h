#ifndef WIDGETS_H
#define WIDGETS_H
#include "../../lib/raylib.h"

// Widgets do painel de propriedades, compartilhados entre editor e relations.

// Aplica no buffer os caracteres digitados no frame, respeitando a capacidade.
void AppendTypedChars(char *buffer, int capacity);

// Desenha o botao e retorna true no frame em que ele e clicado.
bool PanelButton(Rectangle rect, const char *label, bool selected, Color accent, int fontSize, int *cursor);

// Desenha o campo de texto (com cursor piscando quando focado) e retorna true no clique.
bool PanelField(Rectangle rect, const char *value, bool focused, int *cursor);

void PanelLabel(const char *text, float x, float y);

#endif
