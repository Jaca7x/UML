#ifndef UI_H
#define UI_H
#include "../../lib/raylib.h"

void DrawUi(int *cursor);
bool IsMouseOverUi(void);
void ToggleFullscreenMode(void);

// Mensagem temporaria na barra do topo (resultado de salvar/carregar)
void ShowUiStatus(const char *message, bool success);

// O campo de arquivo esta recebendo digitacao: o painel deve ignorar teclas,
// senao o mesmo Backspace apagaria letra nos dois lugares
bool IsFileFieldFocused(void);

#endif
