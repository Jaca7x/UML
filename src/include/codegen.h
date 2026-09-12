#ifndef CODEGEN_H
#define CODEGEN_H
#include "../../lib/raylib.h"

#define CODEGEN_FOLDER "codigo"

// Gera um arquivo .java por classe do diagrama dentro de CODEGEN_FOLDER.
// Devolve quantos arquivos foram escritos em outFileCount.
bool GenerateJavaCode(int *outFileCount);

#endif
