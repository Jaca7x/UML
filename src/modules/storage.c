#include "../include/storage.h"
#include "../include/editor.h"
#include "../include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Formato de linha unica por elemento, com aspas em volta do que o usuario
// digita (nome e tipo aceitam espaco). Os `param` pertencem sempre a ultima
// `class` lida.
//
//   # RayUML 1
//   class <id> <x> <y> <userWidth> <userHeight> "<nome>"
//   param <visibilidade> "<nome>" "<tipo>"
//   relation <origem> <destino> <tipo> "<mult origem>" "<mult destino>"

#define FORMAT_HEADER "# RayUML 1"
#define LINE_LEN 512

// Retrato do que esta gravado em disco. Comparar o diagrama atual com ele diz
// se ha alteracao pendente sem precisar de gancho em cada ponto de edicao.
static char *savedSnapshot = NULL;
static char currentPath[DIAGRAM_PATH_LEN] = DEFAULT_DIAGRAM_PATH;

const char *GetCurrentDiagramPath(void)
{
    return currentPath;
}

void SetCurrentDiagramPath(const char *path)
{
    snprintf(currentPath, sizeof(currentPath), "%s", path);
}

// Exigir que o usuario digite a extensao faz ele perder o arquivo: sem ela o
// arquivo nao aparece na lista de abrir.
static void NormalizeDiagramPath(const char *path, char *out, int outSize)
{
    while (*path == ' ') path++;

    int length = (int)strlen(path);
    while (length > 0 && path[length - 1] == ' ') length--;

    if (length == 0)
    {
        snprintf(out, outSize, "%s", DEFAULT_DIAGRAM_PATH);
        return;
    }

    int extension = (int)strlen(DIAGRAM_EXTENSION);
    bool hasExtension = (length > extension);

    for (int i = 0; i < extension && hasExtension; i++)
    {
        if (tolower(path[length - extension + i]) != DIAGRAM_EXTENSION[i]) hasExtension = false;
    }

    if (hasExtension) snprintf(out, outSize, "%.*s", length, path);
    else snprintf(out, outSize, "%.*s%s", length, path, DIAGRAM_EXTENSION);
}

static void MarkDiagramSaved(void)
{
    free(savedSnapshot);
    savedSnapshot = SerializeDiagram();
}

bool IsDiagramDirty(void)
{
    char *current = SerializeDiagram();
    if (current == NULL) return false;

    // Sem retrato anterior, so esta sujo se houver algum conteudo
    bool dirty = (savedSnapshot == NULL) ? (GetClassCount() > 0)
                                         : (strcmp(savedSnapshot, current) != 0);
    free(current);

    return dirty;
}

// Limite superior por elemento, com folga: evita realocar durante a escrita
#define BYTES_PER_CLASS    2048
#define BYTES_PER_RELATION  128

char *SerializeDiagram(void)
{
    int capacity = 64 + GetClassCount() * BYTES_PER_CLASS + GetRelationCount() * BYTES_PER_RELATION;
    char *text = (char *)malloc(capacity);
    if (text == NULL) return NULL;

    int used = snprintf(text, capacity, "%s\n", FORMAT_HEADER);

    for (int i = 0; i < GetClassCount(); i++)
    {
        const UMLClass *cls = GetClass(i);

        used += snprintf(text + used, capacity - used, "class %d %.2f %.2f %.2f %.2f \"%s\"\n",
                         cls->id, cls->bounds.x, cls->bounds.y, cls->userWidth, cls->userHeight, cls->name);

        for (int p = 0; p < cls->paramCount; p++)
        {
            used += snprintf(text + used, capacity - used, "param %c \"%s\" \"%s\"\n",
                             cls->params[p].visibility, cls->params[p].name, cls->params[p].type);
        }
    }

    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);

        used += snprintf(text + used, capacity - used, "relation %d %d %s \"%s\" \"%s\"\n",
                         relation->fromId, relation->toId, GetRelationTypeKey(relation->type),
                         relation->fromMultiplicity, relation->toMultiplicity);
    }

    return text;
}

bool SaveDiagram(const char *path)
{
    char *text = SerializeDiagram();
    if (text == NULL) return false;

    char normalized[DIAGRAM_PATH_LEN];
    NormalizeDiagramPath(path, normalized, sizeof(normalized));

    bool saved = SaveFileText(normalized, text);
    free(text);

    if (saved)
    {
        SetCurrentDiagramPath(normalized);
        MarkDiagramSaved();
    }

    return saved;
}

static bool ReadClassLine(const char *line, int *lastClassIndex)
{
    int id;
    float x, y, userWidth, userHeight;
    char name[CLASS_NAME_LEN] = {0};

    if (sscanf(line, "class %d %f %f %f %f \"%63[^\"]\"", &id, &x, &y, &userWidth, &userHeight, name) < 5) return false;

    // Largura e altura reais sao recalculadas a partir do conteudo no proximo frame
    Rectangle bounds = {x, y, 0.0f, 0.0f};
    *lastClassIndex = AddClassFromData(id, name, bounds, userWidth, userHeight);

    return true;
}

static bool ReadParamLine(const char *line, int lastClassIndex)
{
    char visibility;
    char name[PARAM_NAME_LEN] = {0};
    char type[PARAM_TYPE_LEN] = {0};

    if (lastClassIndex == -1) return false;
    if (sscanf(line, "param %c \"%47[^\"]\" \"%31[^\"]\"", &visibility, name, type) != 3) return false;

    AddParamToClass(lastClassIndex, visibility, name, type);

    return true;
}

static bool ReadRelationLine(const char *line)
{
    int fromId, toId;
    char key[32] = {0};
    char fromMultiplicity[MULTIPLICITY_LEN] = {0};
    char toMultiplicity[MULTIPLICITY_LEN] = {0};

    // As multiplicidades podem estar vazias, entao nao entram na contagem minima
    if (sscanf(line, "relation %d %d %31s \"%7[^\"]\" \"%7[^\"]\"", &fromId, &toId, key,
               fromMultiplicity, toMultiplicity) < 3) return false;

    AddRelationFromData(fromId, toId, ParseRelationTypeKey(key), fromMultiplicity, toMultiplicity);

    return true;
}

bool DeserializeDiagram(const char *text)
{
    if (text == NULL || strncmp(text, FORMAT_HEADER, strlen(FORMAT_HEADER)) != 0) return false;

    ClearAllClasses();
    ClearAllRelations();

    int lastClassIndex = -1;
    const char *cursor = text;

    while (*cursor != '\0')
    {
        const char *newline = strchr(cursor, '\n');
        int length = (newline != NULL) ? (int)(newline - cursor) : (int)strlen(cursor);
        if (length >= LINE_LEN) length = LINE_LEN - 1;

        char line[LINE_LEN];
        memcpy(line, cursor, length);
        line[length] = '\0';

        if (strncmp(line, "class ", 6) == 0) ReadClassLine(line, &lastClassIndex);
        else if (strncmp(line, "param ", 6) == 0) ReadParamLine(line, lastClassIndex);
        else if (strncmp(line, "relation ", 9) == 0) ReadRelationLine(line);

        if (newline == NULL) break;
        cursor = newline + 1;
    }

    return true;
}

bool LoadDiagram(const char *path)
{
    char *text = LoadFileText((char *)path);
    if (text == NULL) return false;

    bool loaded = DeserializeDiagram(text);
    UnloadFileText(text);

    if (loaded)
    {
        SetCurrentDiagramPath(path);
        MarkDiagramSaved();
    }

    return loaded;
}
