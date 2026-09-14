#include "tests.h"
#include "../src/include/codegen.h"
#include "../src/include/editor.h"
#include "../src/include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Le um arquivo gerado; devolve NULL se nao existir. Quem chama libera com
// UnloadFileText.
static char *ReadGenerated(const char *folder, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.java", folder, name);

    if (!FileExists(path)) return NULL;

    return LoadFileText(path);
}

static void TestGeneratesOneFilePerClass(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    CHECK(GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder)));
    CHECK_INT(files, GetClassCount());

    // com.empresa.app tem de virar pasta, senao o Java nao aceita o arquivo
    CHECK_STR(folder, CODEGEN_FOLDER "/ruml/autoteste");

    for (int i = 0; i < GetClassCount(); i++)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s.java", folder, GetClass(i)->name);

        CHECK(FileExists(path));
    }
}

// Java exige o package antes de qualquer import: na ordem trocada nao compila
static void TestPackageComesBeforeImports(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *source = ReadGenerated(folder, "Dono");
    CHECK(source != NULL);

    if (source != NULL)
    {
        const char *package = strstr(source, "package " TEST_PACKAGE ";");
        const char *import = strstr(source, "import java.util.List;");

        CHECK(package != NULL);
        CHECK(import != NULL);
        CHECK(package != NULL && import != NULL && package < import);

        UnloadFileText(source);
    }
}

static void TestEmptyPackageKeepsFlatFolder(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    CHECK(GenerateJavaCode("", &files, folder, sizeof(folder)));
    CHECK_STR(folder, CODEGEN_FOLDER);

    char *source = ReadGenerated(folder, "Dono");
    CHECK(source != NULL);

    if (source != NULL)
    {
        CHECK_MISSING(source, "package ");
        UnloadFileText(source);
    }

    // Pacote vazio escreve solto em codigo/, que e onde o usuario gera o
    // codigo dele: o teste tira a amostra de la assim que termina
    RemoveSampleJavaFiles(folder);
}

// Espaco nao existe em nome de pacote e viraria pasta invalida
static void TestPackageIsCleaned(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    CHECK(GenerateJavaCode("  ruml.autoteste  ", &files, folder, sizeof(folder)));
    CHECK_STR(folder, CODEGEN_FOLDER "/ruml/autoteste");
}

static void TestClassKindBecomesKeyword(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *pagavel = ReadGenerated(folder, "Pagavel");
    char *animal = ReadGenerated(folder, "Animal");
    char *status = ReadGenerated(folder, "Status");
    char *cachorro = ReadGenerated(folder, "Cachorro");

    CHECK_CONTAINS(pagavel, "public interface Pagavel {");
    CHECK_CONTAINS(animal, "public abstract class Animal {");
    CHECK_CONTAINS(status, "public enum Status {");
    CHECK_CONTAINS(cachorro, "public class Cachorro");

    // Metodo de interface nao tem corpo nem modificador: ja e public abstract
    CHECK_CONTAINS(pagavel, "    void pagar(double valor);");
    CHECK_MISSING(pagavel, "public void pagar");

    // Valor de enum nao e campo
    CHECK_CONTAINS(status, "    ATIVO,");
    CHECK_CONTAINS(status, "    INATIVO;");

    UnloadFileText(pagavel);
    UnloadFileText(animal);
    UnloadFileText(status);
    UnloadFileText(cachorro);
}

static void TestRelationsBecomeCode(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *cachorro = ReadGenerated(folder, "Cachorro");
    char *dono = ReadGenerated(folder, "Dono");

    CHECK_CONTAINS(cachorro, "extends Animal");
    CHECK_CONTAINS(cachorro, "implements Pagavel");

    // Multiplicidade de muitos vira colecao, e a colecao puxa o import
    CHECK_CONTAINS(dono, "private List<Cachorro> cachorros;");
    CHECK_CONTAINS(dono, "import java.util.List;");

    // Dependencia e uso passageiro: nao vira campo
    CHECK_MISSING(dono, "private Status status;");

    UnloadFileText(cachorro);
    UnloadFileText(dono);
}

// Sem os stubs, a classe concreta que implementa interface nao compila
static void TestInterfaceStubs(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *cachorro = ReadGenerated(folder, "Cachorro");

    CHECK_CONTAINS(cachorro, "@Override");
    CHECK_CONTAINS(cachorro, "public void pagar(double valor) {");

    UnloadFileText(cachorro);
}

// O que se escreve no diagrama nem sempre existe em Java
static void TestTypeTranslation(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *cachorro = ReadGenerated(folder, "Cachorro");

    CHECK_CONTAINS(cachorro, "public boolean latir() {");

    // Metodo com retorno precisa devolver algo, ou o arquivo nao compila
    CHECK_CONTAINS(cachorro, "        return false;");

    UnloadFileText(cachorro);
}

// O ~ do UML e o package-private do Java, que se escreve sem modificador.
// Ja saiu como "private", que restringe o acesso de verdade — e o teste de
// ida e volta foi quem denunciou.
static void TestVisibilityMapping(void)
{
    BuildSampleDiagram();

    int files = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder));

    char *dono = ReadGenerated(folder, "Dono");
    char *animal = ReadGenerated(folder, "Animal");

    CHECK_CONTAINS(dono, "    private String cpf;");
    CHECK_CONTAINS(animal, "    protected String nome;");

    // Sem modificador, e sem espaco sobrando no lugar dele
    CHECK_CONTAINS(dono, "    String listar() {");
    CHECK_MISSING(dono, "private String listar");

    UnloadFileText(dono);
    UnloadFileText(animal);
}

static void TestEmptyDiagramGeneratesNothing(void)
{
    ClearAllClasses();
    ClearAllRelations();

    int files = 0;
    char folder[256] = {0};

    CHECK(!GenerateJavaCode(TEST_PACKAGE, &files, folder, sizeof(folder)));
    CHECK_INT(files, 0);
}

void RunCodegenTests(void)
{
    StartSuite("codegen — geracao de Java");

    TestGeneratesOneFilePerClass();
    TestPackageComesBeforeImports();
    TestEmptyPackageKeepsFlatFolder();
    TestPackageIsCleaned();
    TestClassKindBecomesKeyword();
    TestRelationsBecomeCode();
    TestInterfaceStubs();
    TestTypeTranslation();
    TestVisibilityMapping();
    TestEmptyDiagramGeneratesNothing();
}
