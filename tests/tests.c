#include "tests.h"
#include "../src/include/editor.h"
#include "../src/include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *suiteName = "";
static int checksRun = 0;
static int checksFailed = 0;
static int suiteFailed = 0;

void StartSuite(const char *name)
{
    suiteName = name;
    suiteFailed = 0;

    printf("\n%s\n", name);
}

void RecordCheck(bool passed, const char *expression, int line, const char *detail)
{
    checksRun++;

    if (passed) return;

    checksFailed++;
    suiteFailed++;

    printf("  FALHOU  linha %d: %s\n", line, expression);

    if (detail != NULL && detail[0] != '\0') printf("          %s\n", detail);
}

int FinishTests(void)
{
    printf("\n----------------------------------------\n");

    if (checksFailed == 0) printf("%d verificacoes, tudo passou\n", checksRun);
    else printf("%d verificacoes, %d FALHARAM\n", checksRun, checksFailed);

    (void)suiteName;
    (void)suiteFailed;

    return (checksFailed == 0) ? 0 : 1;
}

void BuildSampleDiagram(void)
{
    ClearAllClasses();
    ClearAllRelations();

    Rectangle spot = {100.0f, 100.0f, 0.0f, 0.0f};
    int pagavel = AddClassFromData(1, "Pagavel", CLASS_KIND_INTERFACE, spot, 0.0f, 0.0f);
    AddMethodToClass(pagavel, '+', "pagar", "double valor", "void");

    spot.x = 400.0f;
    int animal = AddClassFromData(2, "Animal", CLASS_KIND_ABSTRACT, spot, 0.0f, 0.0f);
    AddParamToClass(animal, '#', "nome", "String");
    AddMethodToClass(animal, '+', "dormir", "", "void");

    spot.x = 400.0f;
    spot.y = 350.0f;
    int cachorro = AddClassFromData(3, "Cachorro", CLASS_KIND_CLASS, spot, 220.0f, 0.0f);
    AddParamToClass(cachorro, '-', "raca", "String");

    // Retorno bool exercita a traducao de tipo nos dois sentidos
    AddMethodToClass(cachorro, '+', "latir", "", "bool");

    spot.x = 100.0f;
    int dono = AddClassFromData(4, "Dono", CLASS_KIND_CLASS, spot, 0.0f, 0.0f);
    AddParamToClass(dono, '-', "cpf", "String");

    // Argumento vazio no meio de campos preenchidos: o formato de arquivo ja
    // se perdeu nisso uma vez
    AddMethodToClass(dono, '~', "listar", "", "String");

    spot.x = 700.0f;
    spot.y = 100.0f;
    int status = AddClassFromData(5, "Status", CLASS_KIND_ENUM, spot, 0.0f, 0.0f);
    AddParamToClass(status, '+', "ATIVO", "");
    AddParamToClass(status, '+', "INATIVO", "");

    AddRelationFromData(3, 2, RELATION_INHERITANCE, "", "");
    AddRelationFromData(3, 1, RELATION_REALIZATION, "", "");
    AddRelationFromData(4, 3, RELATION_AGGREGATION, "1", "0..*");

    // Dependencia nao gera codigo nenhum: so sobrevive pelo marcador
    AddRelationFromData(4, 5, RELATION_DEPENDENCY, "", "");
}

#define DUMP_CAPACITY 8192

char *DumpDiagramByName(void)
{
    char *text = (char *)malloc(DUMP_CAPACITY);
    if (text == NULL) return NULL;

    int used = 0;
    text[0] = '\0';

    for (int i = 0; i < GetClassCount(); i++)
    {
        const UMLClass *cls = GetClass(i);

        used += snprintf(text + used, DUMP_CAPACITY - used, "%s|KIND|%s\n",
                         cls->name, GetClassKindKey(cls->kind));

        used += snprintf(text + used, DUMP_CAPACITY - used, "%s|POS|%.2f %.2f %.2f %.2f\n",
                         cls->name, cls->bounds.x, cls->bounds.y, cls->userWidth, cls->userHeight);

        for (int p = 0; p < cls->paramCount; p++)
        {
            used += snprintf(text + used, DUMP_CAPACITY - used, "%s|PARAM|%c %s %s\n",
                             cls->name, cls->params[p].visibility,
                             cls->params[p].name, cls->params[p].type);
        }

        for (int m = 0; m < cls->methodCount; m++)
        {
            used += snprintf(text + used, DUMP_CAPACITY - used, "%s|METHOD|%c %s (%s) %s\n",
                             cls->name, cls->methods[m].visibility, cls->methods[m].name,
                             cls->methods[m].args, cls->methods[m].returnType);
        }
    }

    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        int from = FindClassIndexById(relation->fromId);
        int to = FindClassIndexById(relation->toId);

        if (from == -1 || to == -1) continue;

        used += snprintf(text + used, DUMP_CAPACITY - used, "REL|%s->%s|%s|%s|%s\n",
                         GetClass(from)->name, GetClass(to)->name,
                         GetRelationTypeKey(relation->type),
                         relation->fromMultiplicity, relation->toMultiplicity);
    }

    return text;
}

void RemoveSampleJavaFiles(const char *folder)
{
    for (int i = 0; i < GetClassCount(); i++)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s.java", folder, GetClass(i)->name);

        remove(path);
    }
}

static int CountLines(const char *text)
{
    int count = 0;

    for (const char *cursor = text; *cursor != '\0'; cursor++)
    {
        if (*cursor == '\n') count++;
    }

    return count;
}

static bool ContainsLine(const char *text, const char *line, int length)
{
    for (const char *cursor = text; *cursor != '\0'; )
    {
        const char *newline = strchr(cursor, '\n');
        int current = (newline != NULL) ? (int)(newline - cursor) : (int)strlen(cursor);

        if (current == length && strncmp(cursor, line, length) == 0) return true;

        if (newline == NULL) break;
        cursor = newline + 1;
    }

    return false;
}

bool SameDiagramDump(const char *a, const char *b)
{
    if (a == NULL || b == NULL) return false;
    if (CountLines(a) != CountLines(b)) return false;

    for (const char *cursor = a; *cursor != '\0'; )
    {
        const char *newline = strchr(cursor, '\n');
        int length = (newline != NULL) ? (int)(newline - cursor) : (int)strlen(cursor);

        if (length > 0 && !ContainsLine(b, cursor, length))
        {
            printf("  ausente no segundo diagrama: %.*s\n", length, cursor);
            return false;
        }

        if (newline == NULL) break;
        cursor = newline + 1;
    }

    return true;
}

int main(void)
{
    // A raylib fala demais para uma saida de teste
    SetTraceLogLevel(LOG_NONE);

    if (!DirectoryExists(TEST_WORK_DIR)) MakeDirectory(TEST_WORK_DIR);

    printf("RayUML Editor — testes\n");

    RunStorageTests();
    RunCodegenTests();
    RunCodeparseTests();
    RunValidationTests();

    return FinishTests();
}
