#ifndef STORAGE_H
#define STORAGE_H
#include "../../lib/raylib.h"

#define DEFAULT_DIAGRAM_PATH  "diagrama.ruml"
#define RECOVERY_DIAGRAM_PATH "recuperacao.ruml"
#define DIAGRAM_PATH_LEN 128

// Serializa o diagrama inteiro. Devolve buffer alocado (quem chama libera),
// ou NULL se faltar memoria. Usado tambem pelo historico de desfazer.
char *SerializeDiagram(void);

// Substitui o diagrama atual pelo conteudo do texto.
bool DeserializeDiagram(const char *text);

bool SaveDiagram(const char *path);
bool LoadDiagram(const char *path);

// Arquivo em que o diagrama esta sendo editado
const char *GetCurrentDiagramPath(void);
void SetCurrentDiagramPath(const char *path);

// Ha alteracoes que ainda nao foram gravadas
bool IsDiagramDirty(void);

#endif
