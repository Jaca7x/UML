#ifndef UIFONT_H
#define UIFONT_H
#include "../../lib/raylib.h"

// Carrega assets/fonts/ui.ttf. Se o arquivo nao existir, usa a fonte padrao da raylib.
void LoadUiFont(void);
void UnloadUiFont(void);

void DrawUiText(const char *text, float x, float y, int fontSize, Color color);
int MeasureUiText(const char *text, int fontSize);

#endif
