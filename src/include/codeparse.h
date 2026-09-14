#ifndef CODEPARSE_H
#define CODEPARSE_H
#include "../../lib/raylib.h"

// Le os .java de uma pasta e monta o diagrama a partir deles: o caminho de
// volta do codegen. O pacote (pode ser vazio) escolhe a subpasta, igual na
// geracao: com.empresa.app -> codigo/com/empresa/app/.
// outFolder recebe a pasta lida; o diagrama atual e substituido.
bool ImportJavaCode(const char *package, int *outClassCount, char *outFolder, int outFolderSize);

// Mesma leitura, com a pasta ja escolhida: separa achar o caminho de
// interpretar o que ha nele, e e por onde os testes entram.
bool ImportJavaCodeFromFolder(const char *folder, int *outClassCount);

#endif
