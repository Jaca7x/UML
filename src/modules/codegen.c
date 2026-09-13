#include "../include/codegen.h"
#include "../include/editor.h"
#include "../include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE_CAPACITY 8192
#define LINE_CAPACITY    256

// Tipos do diagrama que nao sao validos em Java. O resto passa direto: nome de
// classe usado como tipo tem que sobreviver a traducao.
static const char *MapType(const char *type)
{
    if (strcmp(type, "bool") == 0) return "boolean";
    if (type[0] == '\0') return "void";

    return type;
}

static const char *MapVisibility(char visibility)
{
    if (visibility == '+') return "public";
    if (visibility == '#') return "protected";

    return "private";
}

// Corpo minimo que compila: Java exige retorno em metodo nao-void.
static const char *DefaultReturn(const char *type)
{
    if (strcmp(type, "void") == 0) return NULL;
    if (strcmp(type, "boolean") == 0) return "false";
    if (strcmp(type, "char") == 0) return "'\\0'";

    if (strcmp(type, "int") == 0 || strcmp(type, "long") == 0
     || strcmp(type, "float") == 0 || strcmp(type, "double") == 0) return "0";

    return "null";
}

static const char *KindKeyword(ClassKind kind)
{
    if (kind == CLASS_KIND_INTERFACE) return "interface";
    if (kind == CLASS_KIND_ENUM) return "enum";
    if (kind == CLASS_KIND_ABSTRACT) return "abstract class";

    return "class";
}

// Multiplicidade de muitos vira colecao: 1 fica campo simples.
static bool IsManyMultiplicity(const char *multiplicity)
{
    return strchr(multiplicity, '*') != NULL;
}

static void AppendLine(char *source, int capacity, const char *line)
{
    int used = (int)strlen(source);
    snprintf(source + used, capacity - used, "%s\n", line);
}

// Heranca: no maximo uma em Java, entao a primeira encontrada vence.
static const char *FindParentName(int classId)
{
    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        if (relation->type != RELATION_INHERITANCE || relation->fromId != classId) continue;

        int parentIndex = FindClassIndexById(relation->toId);
        if (parentIndex != -1) return GetClass(parentIndex)->name;
    }

    return NULL;
}

static void AppendDeclaration(char *source, int capacity, const UMLClass *cls)
{
    char line[LINE_CAPACITY];
    char implementsList[LINE_CAPACITY] = {0};

    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        if (relation->type != RELATION_REALIZATION || relation->fromId != cls->id) continue;

        int index = FindClassIndexById(relation->toId);
        if (index == -1) continue;

        int used = (int)strlen(implementsList);
        snprintf(implementsList + used, sizeof(implementsList) - used,
                 "%s%s", (used > 0) ? ", " : "", GetClass(index)->name);
    }

    const char *parent = FindParentName(cls->id);

    snprintf(line, sizeof(line), "public %s %s%s%s%s%s {",
             KindKeyword(cls->kind), cls->name,
             (parent != NULL) ? " extends " : "", (parent != NULL) ? parent : "",
             (implementsList[0] != '\0') ? " implements " : "", implementsList);

    AppendLine(source, capacity, line);
}

// Associacao, agregacao e composicao viram campo do tipo da classe ligada.
// Dependencia nao: ela e uso passageiro, nao posse.
static void AppendRelationFields(char *source, int capacity, const UMLClass *cls)
{
    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        if (relation->fromId != cls->id) continue;

        if (relation->type != RELATION_ASSOCIATION && relation->type != RELATION_AGGREGATION
         && relation->type != RELATION_COMPOSITION) continue;

        int targetIndex = FindClassIndexById(relation->toId);
        if (targetIndex == -1) continue;

        const char *targetName = GetClass(targetIndex)->name;
        char fieldName[LINE_CAPACITY];
        char line[LINE_CAPACITY];

        snprintf(fieldName, sizeof(fieldName), "%s", targetName);
        if (fieldName[0] >= 'A' && fieldName[0] <= 'Z') fieldName[0] += 32;

        // O marcador carrega o que o campo Java sozinho nao diz: agregacao e
        // composicao viram exatamente o mesmo codigo, e a multiplicidade some.
        char mark[LINE_CAPACITY];
        snprintf(mark, sizeof(mark), " // @ruml rel %s %s \"%s\" \"%s\"",
                 GetRelationTypeKey(relation->type), targetName,
                 relation->fromMultiplicity, relation->toMultiplicity);

        if (IsManyMultiplicity(relation->toMultiplicity))
        {
            snprintf(line, sizeof(line), "    private List<%s> %ss;%s", targetName, fieldName, mark);
        }
        else
        {
            snprintf(line, sizeof(line), "    private %s %s;%s", targetName, fieldName, mark);
        }

        AppendLine(source, capacity, line);
    }
}

// Dependencia e uso passageiro, entao nao vira campo — e sem campo nao sobra
// nada no arquivo que a releitura possa reconhecer. So o marcador a segura.
static void AppendDependencyMarkers(char *source, int capacity, const UMLClass *cls)
{
    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        if (relation->type != RELATION_DEPENDENCY || relation->fromId != cls->id) continue;

        int targetIndex = FindClassIndexById(relation->toId);
        if (targetIndex == -1) continue;

        char line[LINE_CAPACITY];
        snprintf(line, sizeof(line), "    // @ruml rel dependencia %s \"%s\" \"%s\"",
                 GetClass(targetIndex)->name, relation->fromMultiplicity, relation->toMultiplicity);

        AppendLine(source, capacity, line);
    }
}

static void AppendEnumValues(char *source, int capacity, const UMLClass *cls)
{
    for (int i = 0; i < cls->paramCount; i++)
    {
        char line[LINE_CAPACITY];
        snprintf(line, sizeof(line), "    %s%s", cls->params[i].name,
                 (i < cls->paramCount - 1) ? "," : ";");

        AppendLine(source, capacity, line);
    }
}

static void AppendFields(char *source, int capacity, const UMLClass *cls)
{
    for (int i = 0; i < cls->paramCount; i++)
    {
        char line[LINE_CAPACITY];

        // Atributo em interface so existe em Java como constante
        if (cls->kind == CLASS_KIND_INTERFACE)
        {
            snprintf(line, sizeof(line), "    %s %s;",
                     MapType(cls->params[i].type), cls->params[i].name);
        }
        else
        {
            snprintf(line, sizeof(line), "    %s %s %s;",
                     MapVisibility(cls->params[i].visibility),
                     MapType(cls->params[i].type), cls->params[i].name);
        }

        AppendLine(source, capacity, line);
    }
}

static void AppendMethods(char *source, int capacity, const UMLClass *cls)
{
    for (int i = 0; i < cls->methodCount; i++)
    {
        const UMLMethod *method = &cls->methods[i];
        const char *returnType = MapType(method->returnType);
        char line[LINE_CAPACITY];

        // Metodo de interface nao tem corpo nem modificador: ja e public abstract
        if (cls->kind == CLASS_KIND_INTERFACE)
        {
            snprintf(line, sizeof(line), "    %s %s(%s);", returnType, method->name, method->args);
            AppendLine(source, capacity, line);
            continue;
        }

        AppendLine(source, capacity, "");

        snprintf(line, sizeof(line), "    %s %s %s(%s) {",
                 MapVisibility(method->visibility), returnType, method->name, method->args);
        AppendLine(source, capacity, line);
        AppendLine(source, capacity, "        // TODO implementar");

        const char *defaultReturn = DefaultReturn(returnType);
        if (defaultReturn != NULL)
        {
            snprintf(line, sizeof(line), "        return %s;", defaultReturn);
            AppendLine(source, capacity, line);
        }

        AppendLine(source, capacity, "    }");
    }
}

static bool HasMethodNamed(const UMLClass *cls, const char *name)
{
    for (int i = 0; i < cls->methodCount; i++)
    {
        if (strcmp(cls->methods[i].name, name) == 0) return true;
    }

    return false;
}

// Classe concreta que implementa interface e obrigada a definir os metodos
// dela: sem os stubs o arquivo gerado nao compila.
static void AppendInterfaceStubs(char *source, int capacity, const UMLClass *cls)
{
    if (cls->kind == CLASS_KIND_INTERFACE || cls->kind == CLASS_KIND_ENUM) return;

    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        if (relation->type != RELATION_REALIZATION || relation->fromId != cls->id) continue;

        int index = FindClassIndexById(relation->toId);
        if (index == -1) continue;

        const UMLClass *contract = GetClass(index);

        for (int m = 0; m < contract->methodCount; m++)
        {
            const UMLMethod *method = &contract->methods[m];
            if (HasMethodNamed(cls, method->name)) continue;

            const char *returnType = MapType(method->returnType);
            char line[LINE_CAPACITY];

            AppendLine(source, capacity, "");
            AppendLine(source, capacity, "    @Override");

            snprintf(line, sizeof(line), "    public %s %s(%s) {", returnType, method->name, method->args);
            AppendLine(source, capacity, line);
            AppendLine(source, capacity, "        // TODO implementar");

            const char *defaultReturn = DefaultReturn(returnType);
            if (defaultReturn != NULL)
            {
                snprintf(line, sizeof(line), "        return %s;", defaultReturn);
                AppendLine(source, capacity, line);
            }

            AppendLine(source, capacity, "    }");
        }
    }
}

static bool NeedsListImport(const UMLClass *cls)
{
    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);
        if (relation->fromId != cls->id) continue;

        if (relation->type == RELATION_ASSOCIATION || relation->type == RELATION_AGGREGATION
         || relation->type == RELATION_COMPOSITION)
        {
            if (IsManyMultiplicity(relation->toMultiplicity)) return true;
        }
    }

    for (int i = 0; i < cls->paramCount; i++)
    {
        if (strcmp(cls->params[i].type, "List") == 0) return true;
    }

    return false;
}

// Espaco nao existe em nome de pacote e viraria pasta invalida
void CleanPackageName(const char *package, char *out, int outSize)
{
    int length = 0;

    for (int i = 0; package[i] != '\0' && length < outSize - 1; i++)
    {
        if (package[i] != ' ') out[length++] = package[i];
    }

    out[length] = '\0';
}

// com.empresa.app -> codigo/com/empresa/app
void BuildPackageFolder(const char *package, char *out, int outSize)
{
    int used = snprintf(out, outSize, "%s", CODEGEN_FOLDER);

    if (package[0] == '\0') return;

    if (used < outSize - 1) out[used++] = '/';

    for (int i = 0; package[i] != '\0' && used < outSize - 1; i++)
    {
        out[used++] = (package[i] == '.') ? '/' : package[i];
    }

    out[used] = '\0';
}

static bool GenerateClassFile(const UMLClass *cls, const char *package, const char *folder)
{
    if (cls->name[0] == '\0') return false;

    char *source = (char *)malloc(SOURCE_CAPACITY);
    if (source == NULL) return false;

    source[0] = '\0';

    AppendLine(source, SOURCE_CAPACITY, "// Gerado pelo RayUML Editor a partir do diagrama.");

    // Java nao guarda onde a caixa estava. Sem esta linha, reabrir o diagrama
    // pelos arquivos rearranjaria tudo em grade a cada vez.
    char marker[LINE_CAPACITY];
    snprintf(marker, sizeof(marker), "// @ruml pos %.2f %.2f %.2f %.2f",
             cls->bounds.x, cls->bounds.y, cls->userWidth, cls->userHeight);
    AppendLine(source, SOURCE_CAPACITY, marker);

    // A declaracao de pacote precisa vir antes de qualquer import
    if (package[0] != '\0')
    {
        char line[LINE_CAPACITY];
        snprintf(line, sizeof(line), "\npackage %s;", package);
        AppendLine(source, SOURCE_CAPACITY, line);
    }

    if (NeedsListImport(cls))
    {
        AppendLine(source, SOURCE_CAPACITY, "");
        AppendLine(source, SOURCE_CAPACITY, "import java.util.List;");
    }

    AppendLine(source, SOURCE_CAPACITY, "");
    AppendDeclaration(source, SOURCE_CAPACITY, cls);

    if (cls->kind == CLASS_KIND_ENUM)
    {
        AppendEnumValues(source, SOURCE_CAPACITY, cls);
    }
    else
    {
        AppendFields(source, SOURCE_CAPACITY, cls);
        AppendRelationFields(source, SOURCE_CAPACITY, cls);
        AppendDependencyMarkers(source, SOURCE_CAPACITY, cls);
        AppendMethods(source, SOURCE_CAPACITY, cls);
        AppendInterfaceStubs(source, SOURCE_CAPACITY, cls);
    }

    AppendLine(source, SOURCE_CAPACITY, "}");

    char path[512];
    snprintf(path, sizeof(path), "%s/%s.java", folder, cls->name);

    bool saved = SaveFileText(path, source);
    free(source);

    return saved;
}

bool GenerateJavaCode(const char *package, int *outFileCount, char *outFolder, int outFolderSize)
{
    *outFileCount = 0;

    if (GetClassCount() == 0) return false;

    char cleaned[PACKAGE_LEN] = {0};
    CleanPackageName(package, cleaned, sizeof(cleaned));

    BuildPackageFolder(cleaned, outFolder, outFolderSize);

    if (!DirectoryExists(outFolder)) MakeDirectory(outFolder);

    for (int i = 0; i < GetClassCount(); i++)
    {
        if (GenerateClassFile(GetClass(i), cleaned, outFolder)) (*outFileCount)++;
    }

    return (*outFileCount) > 0;
}
