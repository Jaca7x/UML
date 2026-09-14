#include "tests.h"
#include "../src/include/storage.h"
#include "../src/include/editor.h"
#include "../src/include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void TestRoundTrip(void)
{
    BuildSampleDiagram();

    char *before = DumpDiagramByName();
    char *text = SerializeDiagram();

    CHECK(text != NULL);
    CHECK(DeserializeDiagram(text));

    char *after = DumpDiagramByName();

    CHECK(SameDiagramDump(before, after));

    free(before);
    free(after);
    free(text);
}

// O %[^"] do sscanf para na primeira string vazia e leva junto todos os campos
// seguintes da linha. Ja custou os argumentos e o retorno de um metodo.
static void TestEmptyQuotedField(void)
{
    ClearAllClasses();
    ClearAllRelations();

    Rectangle spot = {0.0f, 0.0f, 0.0f, 0.0f};
    int banco = AddClassFromData(1, "Banco", CLASS_KIND_CLASS, spot, 0.0f, 0.0f);

    AddMethodToClass(banco, '+', "extrato", "", "String");
    AddParamToClass(banco, '-', "saldo", "");

    char *text = SerializeDiagram();
    CHECK(DeserializeDiagram(text));
    free(text);

    CHECK_INT(GetClassCount(), 1);

    const UMLClass *cls = GetClass(0);

    CHECK_INT(cls->methodCount, 1);
    CHECK_STR(cls->methods[0].name, "extrato");
    CHECK_STR(cls->methods[0].args, "");

    // O campo depois do vazio e o que se perdia
    CHECK_STR(cls->methods[0].returnType, "String");

    CHECK_INT(cls->paramCount, 1);
    CHECK_STR(cls->params[0].name, "saldo");
    CHECK_STR(cls->params[0].type, "");
}

// Relacionamento tem o mesmo formato de aspas e sofria do mesmo problema
static void TestEmptyMultiplicity(void)
{
    ClearAllClasses();
    ClearAllRelations();

    Rectangle spot = {0.0f, 0.0f, 0.0f, 0.0f};

    AddClassFromData(1, "A", CLASS_KIND_CLASS, spot, 0.0f, 0.0f);
    AddClassFromData(2, "B", CLASS_KIND_CLASS, spot, 0.0f, 0.0f);
    AddRelationFromData(1, 2, RELATION_ASSOCIATION, "", "0..*");

    char *text = SerializeDiagram();
    CHECK(DeserializeDiagram(text));
    free(text);

    CHECK_INT(GetRelationCount(), 1);
    CHECK_STR(GetRelation(0)->fromMultiplicity, "");
    CHECK_STR(GetRelation(0)->toMultiplicity, "0..*");
}

// O tipo da classe foi acrescentado depois das aspas do nome justamente para
// nao invalidar arquivo gravado antes de existir
static void TestFileWithoutClassKind(void)
{
    const char *old =
        "# RayUML 1\n"
        "class 1 10.00 20.00 0.00 0.00 \"Antiga\"\n"
        "param - \"campo\" \"int\"\n";

    CHECK(DeserializeDiagram(old));
    CHECK_INT(GetClassCount(), 1);
    CHECK_STR(GetClass(0)->name, "Antiga");
    CHECK_INT((int)GetClass(0)->kind, (int)CLASS_KIND_CLASS);
    CHECK_INT(GetClass(0)->paramCount, 1);
}

// O tipo e gravado por palavra, nao por numero: reordenar o enum nao pode
// mudar o significado de arquivo ja salvo
static void TestKindPersistedByName(void)
{
    BuildSampleDiagram();

    char *text = SerializeDiagram();

    CHECK_CONTAINS(text, "\"Pagavel\" interface");
    CHECK_CONTAINS(text, "\"Animal\" abstrata");
    CHECK_CONTAINS(text, "\"Status\" enum");
    CHECK_CONTAINS(text, "relation 3 2 heranca");

    free(text);
}

static void TestRejectsForeignFile(void)
{
    BuildSampleDiagram();
    int before = GetClassCount();

    CHECK(!DeserializeDiagram("nao sou um diagrama\n"));

    // Recusar tem de ser sem efeito colateral: o diagrama segue de pe
    CHECK_INT(GetClassCount(), before);
}

// Sem isto o arquivo salvo como "teste" nao aparece na lista de abrir e o
// usuario o perde
static void TestExtensionIsCompleted(void)
{
    BuildSampleDiagram();

    CHECK(SaveDiagram(TEST_WORK_DIR "/sem-extensao"));
    CHECK_STR(GetCurrentDiagramPath(), TEST_WORK_DIR "/sem-extensao.ruml");
    CHECK(FileExists(TEST_WORK_DIR "/sem-extensao.ruml"));

    // Ja com extensao, nao duplica
    CHECK(SaveDiagram(TEST_WORK_DIR "/com-extensao.ruml"));
    CHECK_STR(GetCurrentDiagramPath(), TEST_WORK_DIR "/com-extensao.ruml");
}

static void TestSaveThenLoad(void)
{
    BuildSampleDiagram();
    char *before = DumpDiagramByName();

    CHECK(SaveDiagram(TEST_WORK_DIR "/ida-e-volta.ruml"));

    ClearAllClasses();
    ClearAllRelations();
    CHECK_INT(GetClassCount(), 0);

    CHECK(LoadDiagram(TEST_WORK_DIR "/ida-e-volta.ruml"));

    char *after = DumpDiagramByName();
    CHECK(SameDiagramDump(before, after));

    // Recem carregado, nada esta pendente
    CHECK(!IsDiagramDirty());

    free(before);
    free(after);
}

static void TestDirtyDetection(void)
{
    BuildSampleDiagram();

    CHECK(SaveDiagram(TEST_WORK_DIR "/sujeira.ruml"));
    CHECK(!IsDiagramDirty());

    Rectangle spot = {0.0f, 0.0f, 0.0f, 0.0f};
    AddClassFromData(99, "Nova", CLASS_KIND_CLASS, spot, 0.0f, 0.0f);

    CHECK(IsDiagramDirty());
}

static void TestMissingFile(void)
{
    CHECK(!LoadDiagram(TEST_WORK_DIR "/nao-existe.ruml"));
}

void RunStorageTests(void)
{
    StartSuite("storage — formato de arquivo");

    TestRoundTrip();
    TestEmptyQuotedField();
    TestEmptyMultiplicity();
    TestFileWithoutClassKind();
    TestKindPersistedByName();
    TestRejectsForeignFile();
    TestExtensionIsCompleted();
    TestSaveThenLoad();
    TestDirtyDetection();
    TestMissingFile();
}
