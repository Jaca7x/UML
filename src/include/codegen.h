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

// Compartilhados com a importacao, que precisa chegar na mesma pasta que a
// geracao escreveu — se as duas montarem o caminho por conta propria, uma
// mudanca de regra aqui deixa a outra lendo o lugar errado.
void CleanPackageName(const char *package, char *out, int outSize);
void BuildPackageFolder(const char *package, char *out, int outSize);

#endif
