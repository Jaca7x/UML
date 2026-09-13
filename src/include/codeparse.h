#ifndef CODEPARSE_H
#define CODEPARSE_H
#include "../../lib/raylib.h"

// Le os .java de uma pasta e monta o diagrama a partir deles: o caminho de
// volta do codegen. O pacote (pode ser vazio) escolhe a subpasta, igual na
// geracao: com.empresa.app -> codigo/com/empresa/app/.
// outFolder recebe a pasta lida; o diagrama atual e substituido.
bool ImportJavaCode(const char *package, int *outClassCount, char *outFolder, int outFolderSize);

#endif
