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

// --- Usados pelo codegen para nao apagar o que foi escrito a mao ---

// Corpo do metodo com esta assinatura dentro do texto de um .java, sem as
// chaves e com a indentacao original. A assinatura e nome + argumentos: o
// tipo de retorno de proposito nao entra, para que troca-lo preserve o corpo
// e deixe o compilador apontar o que precisa mudar, em vez de o gerador
// apagar codigo em silencio.
bool FindJavaMethodBody(const char *source, const char *name, const char *args,
                        char *out, int outSize);

// Linhas de import que o arquivo ja tinha. Sem elas, o corpo preservado que
// usasse qualquer outra classe deixaria de compilar na proxima geracao.
void CollectImports(const char *source, char *out, int outSize);

#endif
