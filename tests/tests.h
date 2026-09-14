#ifndef TESTS_H
#define TESTS_H
#include "../lib/raylib.h"

// Arnes minimo de teste: sem dependencia externa, porque trazer um framework
// de teste para C neste projeto custaria mais configuracao do que o proprio
// codigo testado.
//
// Os testes rodam sem abrir janela. As funcoes de arquivo da raylib
// (LoadFileText, DirectoryExists, LoadDirectoryFiles) funcionam sem
// InitWindow, que e o que permite verificar salvar/carregar e a geracao de
// codigo no terminal e, mais tarde, na CI.

void StartSuite(const char *name);
void RecordCheck(bool passed, const char *expression, int line, const char *detail);
int FinishTests(void);

#define CHECK(condition) RecordCheck((condition), #condition, __LINE__, "")

#define CHECK_INT(actual, expected)                                              \
    do {                                                                         \
        int actualValue = (actual);                                              \
        int expectedValue = (expected);                                          \
        char detail[128];                                                        \
        snprintf(detail, sizeof(detail), "obtido %d, esperado %d",               \
                 actualValue, expectedValue);                                    \
        RecordCheck(actualValue == expectedValue, #actual, __LINE__, detail);    \
    } while (0)

#define CHECK_STR(actual, expected)                                              \
    do {                                                                         \
        const char *actualValue = (actual);                                      \
        const char *expectedValue = (expected);                                  \
        char detail[256];                                                        \
        snprintf(detail, sizeof(detail), "obtido \"%s\", esperado \"%s\"",       \
                 actualValue, expectedValue);                                    \
        RecordCheck(strcmp(actualValue, expectedValue) == 0, #actual,            \
                    __LINE__, detail);                                           \
    } while (0)

// Trecho presente no texto, para conferir codigo gerado sem depender da
// formatacao exata das linhas em volta
#define CHECK_CONTAINS(text, piece)                                              \
    do {                                                                         \
        const char *haystack = (text);                                           \
        char detail[256];                                                        \
        snprintf(detail, sizeof(detail), "nao encontrei \"%s\"", (piece));       \
        RecordCheck(haystack != NULL && strstr(haystack, (piece)) != NULL,       \
                    #piece, __LINE__, detail);                                   \
    } while (0)

#define CHECK_MISSING(text, piece)                                               \
    do {                                                                         \
        const char *haystack = (text);                                           \
        char detail[256];                                                        \
        snprintf(detail, sizeof(detail), "nao devia conter \"%s\"", (piece));    \
        RecordCheck(haystack != NULL && strstr(haystack, (piece)) == NULL,       \
                    #piece, __LINE__, detail);                                   \
    } while (0)

// Pasta de trabalho dos testes, apagada no inicio de cada execucao
#define TEST_WORK_DIR "build/teste"

// Pacote usado pelos testes de geracao. Proprio, para nao passar por cima do
// codigo que o usuario gerou em codigo/.
#define TEST_PACKAGE "ruml.autoteste"

// Diagrama de referencia: cobre os quatro tipos de classe, visibilidades
// diferentes, metodo sem argumento e os quatro relacionamentos que o gerador
// trata de formas distintas.
void BuildSampleDiagram(void);

// Retrato do diagrama por nome, nao por id: a importacao renumera as classes
// e as le em ordem alfabetica, entao comparar id ou ordem acusaria diferenca
// que nao existe. O chamador libera com free().
char *DumpDiagramByName(void);

// Mesmo conjunto de linhas, em qualquer ordem
bool SameDiagramDump(const char *a, const char *b);

// Os testes geram nas pastas de saida de verdade. Isto remove o que eles
// escreveram, para nao deixar arquivo de amostra no lugar do codigo do
// usuario — apaga pelos nomes do diagrama atual, nunca a pasta inteira.
void RemoveSampleJavaFiles(const char *folder);

void RunStorageTests(void);
void RunCodegenTests(void);
void RunCodeparseTests(void);
void RunValidationTests(void);
void RunPreserveTests(void);

#endif
