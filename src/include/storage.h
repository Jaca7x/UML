#ifndef STORAGE_H
#define STORAGE_H
#include "../../lib/raylib.h"

#define DEFAULT_DIAGRAM_PATH "diagrama.ruml"

// Serializa o diagrama inteiro. Devolve buffer alocado (quem chama libera),
// ou NULL se faltar memoria. Usado tambem pelo historico de desfazer.
char *SerializeDiagram(void);

// Substitui o diagrama atual pelo conteudo do texto.
bool DeserializeDiagram(const char *text);

bool SaveDiagram(const char *path);
bool LoadDiagram(const char *path);

#endif
