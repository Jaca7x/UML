#ifndef STORAGE_H
#define STORAGE_H
#include "../../lib/raylib.h"

#define DEFAULT_DIAGRAM_PATH "diagrama.ruml"

// Grava o diagrama inteiro (classes, parametros e relacionamentos).
bool SaveDiagram(const char *path);

// Substitui o diagrama atual pelo conteudo do arquivo.
bool LoadDiagram(const char *path);

#endif
