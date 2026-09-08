#ifndef HISTORY_H
#define HISTORY_H
#include "../../lib/raylib.h"

// Historico de desfazer por retrato: cada passo guarda o diagrama inteiro
// serializado. Chame PushHistory ANTES de alterar qualquer coisa.
void PushHistory(void);
void UndoHistory(void);
bool CanUndo(void);

#endif
