#include "tests.h"
#include "../src/include/codegen.h"
#include "../src/include/editor.h"
#include "../src/include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Importar convidou a mexer no codigo gerado, e gerar de novo sobrescrevia o
// arquivo inteiro. Estes testes cobrem o que o usuario escreveu a mao.

#define EDIT_CAPACITY 16384

// Simula o usuario abrindo o arquivo no editor e trocando um trecho
static bool EditFile(const char *path, const char *from, const char *to)
{
    char *source = LoadFileText((char *)path);
    if (source == NULL) return false;

    const char *found = strstr(source, from);

    if (found == NULL)
    {
        UnloadFileText(source);
        return false;
    }

    char edited[EDIT_CAPACITY];
    int prefix = (int)(found - source);

    snprintf(edited, sizeof(edited), "%.*s%s%s", prefix, source, to, found + strlen(from));

    bool saved = SaveFileText((char *)path, edited);

    UnloadFileText(source);

    return saved;
}

static void PathTo(const char *folder, const char *name, char *out, int outSize)
{
    snprintf(out, outSize, "%s/%s.java", folder, name);
}

static void TestMethodBodySurvives(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char path[512];
    PathTo(folder, "Cachorro", path, sizeof(path));

    CHECK(EditFile(path,
        "        // TODO implementar\n        return false;",
        "        System.out.println(\"Au au\");\n        return true;"));

    // Segunda geracao, sem mudar nada no diagrama
    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *again = LoadFileText(path);

    CHECK_CONTAINS(again, "System.out.println(\"Au au\");");
    CHECK_CONTAINS(again, "        return true;");

    // E a assinatura continua vindo do diagrama
    CHECK_CONTAINS(again, "public boolean latir() {");

    UnloadFileText(again);
}

// Implementar a interface e justamente o que se faz a mao no stub
static void TestOverrideStubBodySurvives(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char path[512];
    PathTo(folder, "Cachorro", path, sizeof(path));

    char *source = LoadFileText(path);
    const char *stub = strstr(source, "public void pagar(double valor) {");

    CHECK(stub != NULL);
    UnloadFileText(source);

    CHECK(EditFile(path,
        "public void pagar(double valor) {\n        // TODO implementar",
        "public void pagar(double valor) {\n        this.saldo -= valor;"));

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *again = LoadFileText(path);

    CHECK_CONTAINS(again, "this.saldo -= valor;");
    CHECK_CONTAINS(again, "@Override");

    UnloadFileText(again);
}

// O corpo preservado costuma depender de classes que o gerador nao conhece
static void TestImportsSurvive(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char path[512];
    PathTo(folder, "Dono", path, sizeof(path));

    CHECK(EditFile(path, "import java.util.List;",
                         "import java.util.List;\nimport java.util.Map;"));

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *again = LoadFileText(path);

    CHECK_CONTAINS(again, "import java.util.Map;");
    CHECK_CONTAINS(again, "import java.util.List;");

    // Sem duplicar o que o proprio gerador escreve
    if (again != NULL)
    {
        const char *first = strstr(again, "import java.util.List;");
        const char *second = (first != NULL) ? strstr(first + 1, "import java.util.List;") : NULL;

        CHECK(second == NULL);
    }

    UnloadFileText(again);
}

// A estrutura continua vindo do diagrama: so o corpo pertence ao codigo
static void TestDiagramStillOwnsStructure(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char path[512];
    PathTo(folder, "Cachorro", path, sizeof(path));

    EditFile(path, "        // TODO implementar\n        return false;",
                   "        return true;");

    // Metodo novo no diagrama entra; o que ja havia mantem o corpo
    int cachorro = -1;

    for (int i = 0; i < GetClassCount(); i++)
    {
        if (strcmp(GetClass(i)->name, "Cachorro") == 0) cachorro = i;
    }

    CHECK(cachorro != -1);

    if (cachorro == -1) return;

    AddMethodToClass(cachorro, '+', "correr", "int metros", "void");

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *again = LoadFileText(path);

    CHECK_CONTAINS(again, "public void correr(int metros) {");
    CHECK_CONTAINS(again, "        return true;");

    UnloadFileText(again);
}

// Metodo tirado do diagrama some do arquivo: a estrutura e do diagrama, e
// deixar codigo orfao seria pior do que remove-lo
static void TestRemovedMethodDisappears(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char path[512];
    PathTo(folder, "Animal", path, sizeof(path));

    char *before = LoadFileText(path);
    CHECK_CONTAINS(before, "public void dormir() {");
    UnloadFileText(before);

    // Reconstroi o diagrama sem o dormir()
    ClearAllClasses();
    ClearAllRelations();

    Rectangle spot = {400.0f, 100.0f, 0.0f, 0.0f};
    AddClassFromData(2, "Animal", CLASS_KIND_ABSTRACT, spot, 0.0f, 0.0f);
    AddParamToClass(0, '#', "nome", "String");
    AddMethodToClass(0, '+', "acordar", "", "void");

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *after = LoadFileText(path);

    CHECK_CONTAINS(after, "public void acordar() {");
    CHECK_MISSING(after, "public void dormir() {");

    UnloadFileText(after);

    BuildSampleDiagram();
    RemoveSampleJavaFiles(folder);
}

void RunPreserveTests(void)
{
    StartSuite("codegen — preservacao do codigo escrito a mao");

    TestMethodBodySurvives();
    TestOverrideStubBodySurvives();
    TestImportsSurvive();
    TestDiagramStillOwnsStructure();
    TestRemovedMethodDisappears();
}
