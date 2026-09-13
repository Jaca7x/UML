#ifndef CODEGEN_H
#define CODEGEN_H
#include "../../lib/raylib.h"

#define CODEGEN_FOLDER "codigo"
#define PACKAGE_LEN 96

// Gera um arquivo .java por classe do diagrama. O pacote (pode ser vazio) vira
// tanto a declaracao no topo do arquivo quanto a arvore de pastas que o Java
// espera: com.empresa.app -> codigo/com/empresa/app/.
// outFolder recebe a pasta onde os arquivos foram escritos.
bool GenerateJavaCode(const char *package, int *outFileCount, char *outFolder, int outFolderSize);

#endif
