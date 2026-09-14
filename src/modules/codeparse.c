#include "../include/codeparse.h"
#include "../include/codegen.h"
#include "../include/editor.h"
#include "../include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Caminho de volta do codegen: le os .java e remonta o diagrama.
//
// Nao e um parser de Java, e um leitor de linha tolerante. Reconhece o que o
// gerador escreve e a forma comum de escrever o resto a mao; declaracao
// quebrada em varias linhas, classe aninhada e generico com espaco dentro
// (Map<String, Integer>) ficam de fora.
//
// O que Java nao sabe dizer viaja em marcador de comentario:
//   // @ruml pos <x> <y> <larguraUsuario> <alturaUsuario>
//   // @ruml rel <tipo> <ClasseAlvo> "<multOrigem>" "<multDestino>"
// Arquivo sem marcador ainda abre: a posicao cai numa grade automatica e o
// campo de tipo conhecido vira associacao (agregacao, se for colecao).

#define LINE_CAPACITY 512
#define MARKER_POSITION "@ruml pos "
#define MARKER_RELATION "@ruml rel "

// Grade de entrada para classe sem marcador de posicao
#define LAYOUT_COLUMNS  4
#define LAYOUT_ORIGIN   80.0f
#define LAYOUT_STEP_X   260.0f
#define LAYOUT_STEP_Y   220.0f

static bool IsWordChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_';
}

static bool StartsWith(const char *text, const char *prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static void TrimSpaces(const char *src, char *out, int outSize)
{
    while (*src == ' ' || *src == '\t') src++;

    int length = (int)strlen(src);

    while (length > 0 && (src[length - 1] == ' ' || src[length - 1] == '\t'
                       || src[length - 1] == '\r')) length--;

    if (length > outSize - 1) length = outSize - 1;

    memcpy(out, src, length);
    out[length] = '\0';
}

// Palavra inteira: sem isto "class" casaria dentro de "classList"
static const char *FindWord(const char *text, const char *word)
{
    int length = (int)strlen(word);

    for (const char *cursor = strstr(text, word); cursor != NULL; cursor = strstr(cursor + 1, word))
    {
        bool leftFree = (cursor == text) || !IsWordChar(cursor[-1]);

        if (leftFree && !IsWordChar(cursor[length])) return cursor;
    }

    return NULL;
}

static void CopyWord(const char *cursor, char *out, int outSize)
{
    while (*cursor == ' ') cursor++;

    int length = 0;
    while (IsWordChar(*cursor) && length < outSize - 1) out[length++] = *cursor++;

    out[length] = '\0';
}

static void LastToken(const char *text, char *out, int outSize)
{
    const char *space = strrchr(text, ' ');

    snprintf(out, outSize, "%s", (space != NULL) ? space + 1 : text);
}

static void HeadBeforeLastToken(const char *text, char *out, int outSize)
{
    const char *space = strrchr(text, ' ');
    int length = (space != NULL) ? (int)(space - text) : 0;

    if (length > outSize - 1) length = outSize - 1;

    memcpy(out, text, length);
    out[length] = '\0';
}

static void ExtractQuoted(const char *text, int index, char *out, int outSize)
{
    out[0] = '\0';

    for (int i = 0; i <= index; i++)
    {
        text = strchr(text, '"');
        if (text == NULL) return;
        text++;

        const char *end = strchr(text, '"');
        if (end == NULL) return;

        if (i == index)
        {
            int length = (int)(end - text);
            if (length > outSize - 1) length = outSize - 1;

            memcpy(out, text, length);
            out[length] = '\0';

            return;
        }

        text = end + 1;
    }
}

static const char *NextLine(const char *cursor, char *out, int outSize)
{
    if (cursor == NULL || *cursor == '\0') return NULL;

    const char *newline = strchr(cursor, '\n');
    int length = (newline != NULL) ? (int)(newline - cursor) : (int)strlen(cursor);

    if (length > outSize - 1) length = outSize - 1;

    memcpy(out, cursor, length);
    out[length] = '\0';

    return (newline != NULL) ? newline + 1 : cursor + strlen(cursor);
}

// Chave dentro de comentario de fim de linha nao abre nem fecha escopo
static int CountBraces(const char *line)
{
    int delta = 0;

    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == '/' && line[i + 1] == '/') break;

        if (line[i] == '{') delta++;
        else if (line[i] == '}') delta--;
    }

    return delta;
}

// A paleta de tipos do diagrama usa a forma curta; o gerador faz a traducao
// contraria na saida, entao sem isto um ciclo de ida e volta trocaria o tipo.
static const char *MapTypeBack(const char *type)
{
    if (strcmp(type, "boolean") == 0) return "bool";

    return type;
}

static int FindClassIdByName(const char *name)
{
    for (int i = 0; i < GetClassCount(); i++)
    {
        if (strcmp(GetClass(i)->name, name) == 0) return GetClass(i)->id;
    }

    return -1;
}

static int FindClassIndexByName(const char *name)
{
    for (int i = 0; i < GetClassCount(); i++)
    {
        if (strcmp(GetClass(i)->name, name) == 0) return i;
    }

    return -1;
}

static bool IsModifier(const char *word)
{
    static const char *modifiers[] = {
        "public", "private", "protected", "static",
        "final", "abstract", "synchronized", "native", "default"
    };

    int count = sizeof(modifiers) / sizeof(modifiers[0]);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(word, modifiers[i]) == 0) return true;
    }

    return false;
}

// Membro de interface e publico por definicao; fora dela, a ausencia de
// modificador e o package-private do Java, que em UML e o ~
static char VisibilityFromModifiers(const char *text, ClassKind kind)
{
    if (FindWord(text, "public") != NULL) return '+';
    if (FindWord(text, "protected") != NULL) return '#';
    if (FindWord(text, "private") != NULL) return '-';

    return (kind == CLASS_KIND_INTERFACE) ? '+' : '~';
}

// List<Cachorro> -> Cachorro
static bool SplitCollectionType(const char *type, char *out, int outSize)
{
    const char *open = strchr(type, '<');
    if (open == NULL) return false;

    CopyWord(open + 1, out, outSize);

    return out[0] != '\0';
}

static const struct
{
    const char *keyword;
    ClassKind kind;
}typeKeywords[] = {
    {"interface", CLASS_KIND_INTERFACE},
    {"enum",      CLASS_KIND_ENUM},
    {"class",     CLASS_KIND_CLASS}
};

// "public abstract class Animal extends Ser implements Pagavel {"
static bool ParseTypeDeclaration(const char *line, char *name, int nameSize, ClassKind *kind)
{
    // Sem chave e declaracao de variavel, nao de tipo: "Class<?> c = ..."
    if (strchr(line, '{') == NULL) return false;

    int count = sizeof(typeKeywords) / sizeof(typeKeywords[0]);

    for (int i = 0; i < count; i++)
    {
        const char *cursor = FindWord(line, typeKeywords[i].keyword);
        if (cursor == NULL) continue;

        *kind = typeKeywords[i].kind;

        if (*kind == CLASS_KIND_CLASS && FindWord(line, "abstract") != NULL)
        {
            *kind = CLASS_KIND_ABSTRACT;
        }

        CopyWord(cursor + strlen(typeKeywords[i].keyword), name, nameSize);

        return name[0] != '\0';
    }

    return false;
}

// extends e implements ja dizem tudo o que heranca e realizacao precisam:
// nao ha o que guardar em marcador para estes dois.
static void ParseInheritance(const char *line, int classId)
{
    const char *cursor = FindWord(line, "extends");

    if (cursor != NULL)
    {
        char parent[CLASS_NAME_LEN];
        CopyWord(cursor + strlen("extends"), parent, sizeof(parent));

        int parentId = FindClassIdByName(parent);

        if (parentId != -1 && parentId != classId)
        {
            AddRelationFromData(classId, parentId, RELATION_INHERITANCE, "", "");
        }
    }

    cursor = FindWord(line, "implements");
    if (cursor == NULL) return;

    cursor += strlen("implements");

    // A lista vai ate a chave: "implements Pagavel, Serializable {"
    while (*cursor != '\0' && *cursor != '{')
    {
        char contract[CLASS_NAME_LEN];
        CopyWord(cursor, contract, sizeof(contract));

        if (contract[0] == '\0') break;

        int contractId = FindClassIdByName(contract);

        if (contractId != -1 && contractId != classId)
        {
            AddRelationFromData(classId, contractId, RELATION_REALIZATION, "", "");
        }

        while (*cursor == ' ') cursor++;
        while (IsWordChar(*cursor)) cursor++;
        while (*cursor == ',' || *cursor == ' ') cursor++;
    }
}

// "agregacao Cachorro "1" "*"" — o que sobrou do relacionamento original
static void ParseRelationMarker(int classId, const char *marker)
{
    char typeKey[32] = {0};
    char target[CLASS_NAME_LEN] = {0};

    if (sscanf(marker, "%31s %63s", typeKey, target) != 2) return;

    int targetId = FindClassIdByName(target);
    if (targetId == -1) return;

    char fromMultiplicity[MULTIPLICITY_LEN] = {0};
    char toMultiplicity[MULTIPLICITY_LEN] = {0};

    ExtractQuoted(marker, 0, fromMultiplicity, sizeof(fromMultiplicity));
    ExtractQuoted(marker, 1, toMultiplicity, sizeof(toMultiplicity));

    AddRelationFromData(classId, targetId, ParseRelationTypeKey(typeKey),
                        fromMultiplicity, toMultiplicity);
}

static void ParseField(int classIndex, ClassKind kind, const char *line)
{
    char declaration[LINE_CAPACITY];
    snprintf(declaration, sizeof(declaration), "%s", line);

    // O valor inicial e o ponto e virgula nao fazem parte da assinatura
    char *cut = strpbrk(declaration, "=;");
    if (cut != NULL) *cut = '\0';

    char trimmed[LINE_CAPACITY];
    TrimSpaces(declaration, trimmed, sizeof(trimmed));

    char head[LINE_CAPACITY];
    HeadBeforeLastToken(trimmed, head, sizeof(head));

    char name[PARAM_NAME_LEN];
    LastToken(trimmed, name, sizeof(name));

    if (name[0] == '\0' || head[0] == '\0') return;

    char type[PARAM_TYPE_LEN];
    LastToken(head, type, sizeof(type));

    // Campo cujo tipo e uma classe do diagrama e relacionamento, nao atributo:
    // e assim que o gerador materializa associacao, agregacao e composicao.
    // So vale para arquivo sem marcador — o gerador anota o tipo exato.
    char inner[CLASS_NAME_LEN];
    bool isCollection = SplitCollectionType(type, inner, sizeof(inner));
    int targetId = FindClassIdByName(isCollection ? inner : type);

    if (targetId != -1 && targetId != GetClass(classIndex)->id)
    {
        AddRelationFromData(GetClass(classIndex)->id, targetId,
                            isCollection ? RELATION_AGGREGATION : RELATION_ASSOCIATION,
                            "1", isCollection ? "*" : "1");
        return;
    }

    AddParamToClass(classIndex, VisibilityFromModifiers(head, kind), name, MapTypeBack(type));
}

static void ParseMethod(int classIndex, ClassKind kind, const char *line)
{
    const char *open = strchr(line, '(');
    const char *close = strrchr(line, ')');

    if (open == NULL || close == NULL || close < open) return;

    char args[METHOD_ARGS_LEN];
    int argsLength = (int)(close - open) - 1;

    if (argsLength < 0) argsLength = 0;
    if (argsLength > METHOD_ARGS_LEN - 1) argsLength = METHOD_ARGS_LEN - 1;

    memcpy(args, open + 1, argsLength);
    args[argsLength] = '\0';

    char head[LINE_CAPACITY];
    int headLength = (int)(open - line);

    if (headLength > LINE_CAPACITY - 1) headLength = LINE_CAPACITY - 1;

    memcpy(head, line, headLength);
    head[headLength] = '\0';

    char trimmed[LINE_CAPACITY];
    TrimSpaces(head, trimmed, sizeof(trimmed));

    char modifiers[LINE_CAPACITY];
    HeadBeforeLastToken(trimmed, modifiers, sizeof(modifiers));

    char name[METHOD_NAME_LEN];
    LastToken(trimmed, name, sizeof(name));

    char returnType[METHOD_TYPE_LEN];
    LastToken(modifiers, returnType, sizeof(returnType));

    // Construtor nao tem tipo de retorno, e o modelo do diagrama nao tem onde
    // guardar um: o que sobraria seria um metodo de retorno vazio.
    if (name[0] == '\0' || returnType[0] == '\0' || IsModifier(returnType)) return;

    AddMethodToClass(classIndex, VisibilityFromModifiers(modifiers, kind), name,
                     args, MapTypeBack(returnType));
}

// "ATIVO," ou "INATIVO;" — valor de enum e uma palavra sozinha na linha
static bool ParseEnumValue(int classIndex, const char *line)
{
    if (strchr(line, '(') != NULL) return false;

    char value[PARAM_NAME_LEN];
    CopyWord(line, value, sizeof(value));

    if (value[0] == '\0') return false;

    const char *after = line + strlen(value);
    while (*after == ' ') after++;

    if (*after != ',' && *after != ';' && *after != '\0') return false;

    AddParamToClass(classIndex, '+', value, "");

    return true;
}

static void ParseMemberLine(int classIndex, ClassKind kind, const char *line, bool *skipNext)
{
    const char *marker = strstr(line, MARKER_RELATION);

    if (marker != NULL)
    {
        ParseRelationMarker(GetClass(classIndex)->id, marker + strlen(MARKER_RELATION));
        return;
    }

    if (line[0] == '\0' || StartsWith(line, "//")) return;

    if (line[0] == '@')
    {
        // Metodo marcado com @Override cumpre contrato herdado, nao e membro
        // proprio: reimportar criaria na classe um metodo que o diagrama nao
        // tinha, justamente o que o gerador acrescentou por conta propria.
        if (FindWord(line, "Override") != NULL) *skipNext = true;

        return;
    }

    bool skip = *skipNext;
    *skipNext = false;

    if (skip) return;

    if (kind == CLASS_KIND_ENUM && ParseEnumValue(classIndex, line)) return;

    if (strchr(line, '(') != NULL) ParseMethod(classIndex, kind, line);
    else if (strchr(line, ';') != NULL) ParseField(classIndex, kind, line);
}

static void StripSpaces(const char *src, char *out, int outSize)
{
    int length = 0;

    for (int i = 0; src[i] != '\0' && length < outSize - 1; i++)
    {
        if (src[i] != ' ' && src[i] != '\t') out[length++] = src[i];
    }

    out[length] = '\0';
}

// "    public boolean latir() {" bate com latir + ""
static bool MatchesSignature(const char *line, const char *name, const char *args)
{
    const char *open = strchr(line, '(');
    const char *close = strrchr(line, ')');

    if (open == NULL || close == NULL || close < open) return false;
    if (strchr(line, '{') == NULL) return false;

    char head[LINE_CAPACITY];
    int headLength = (int)(open - line);

    if (headLength > LINE_CAPACITY - 1) headLength = LINE_CAPACITY - 1;

    memcpy(head, line, headLength);
    head[headLength] = '\0';

    char trimmed[LINE_CAPACITY];
    TrimSpaces(head, trimmed, sizeof(trimmed));

    char found[METHOD_NAME_LEN];
    LastToken(trimmed, found, sizeof(found));

    if (strcmp(found, name) != 0) return false;

    char foundArgs[LINE_CAPACITY];
    int argsLength = (int)(close - open) - 1;

    if (argsLength < 0) argsLength = 0;
    if (argsLength > LINE_CAPACITY - 1) argsLength = LINE_CAPACITY - 1;

    memcpy(foundArgs, open + 1, argsLength);
    foundArgs[argsLength] = '\0';

    // Espaco nao muda a assinatura, e o usuario pode ter reformatado a linha
    char normalizedFound[LINE_CAPACITY];
    char normalizedWanted[LINE_CAPACITY];

    StripSpaces(foundArgs, normalizedFound, sizeof(normalizedFound));
    StripSpaces(args, normalizedWanted, sizeof(normalizedWanted));

    return strcmp(normalizedFound, normalizedWanted) == 0;
}

bool FindJavaMethodBody(const char *source, const char *name, const char *args,
                        char *out, int outSize)
{
    out[0] = '\0';

    if (source == NULL) return false;

    int depth = 0;
    bool inComment = false;
    bool collecting = false;
    int used = 0;

    const char *cursor = source;
    char raw[LINE_CAPACITY];

    while ((cursor = NextLine(cursor, raw, sizeof(raw))) != NULL)
    {
        char line[LINE_CAPACITY];
        TrimSpaces(raw, line, sizeof(line));

        if (inComment)
        {
            if (strstr(line, "*/") != NULL) inComment = false;
            continue;
        }

        if (!collecting && StartsWith(line, "/*"))
        {
            if (strstr(line, "*/") == NULL) inComment = true;
            continue;
        }

        if (collecting)
        {
            int after = depth + CountBraces(line);

            // A chave que fecha o metodo encerra o corpo e nao faz parte dele
            if (after <= 1) return true;

            int length = (int)strlen(raw);

            if (used + length + 1 < outSize)
            {
                memcpy(out + used, raw, length);
                used += length;
                out[used++] = '\n';
                out[used] = '\0';
            }

            depth = after;
            continue;
        }

        if (depth == 1 && MatchesSignature(line, name, args))
        {
            collecting = true;
            depth += CountBraces(line);
            continue;
        }

        depth += CountBraces(line);
    }

    // Chegou ao fim sem fechar: arquivo truncado, melhor descartar o que veio
    return false;
}

void CollectImports(const char *source, char *out, int outSize)
{
    out[0] = '\0';

    if (source == NULL) return;

    int used = 0;
    const char *cursor = source;
    char raw[LINE_CAPACITY];

    while ((cursor = NextLine(cursor, raw, sizeof(raw))) != NULL)
    {
        char line[LINE_CAPACITY];
        TrimSpaces(raw, line, sizeof(line));

        // Os imports ficam todos antes da declaracao do tipo
        if (strchr(line, '{') != NULL) return;
        if (!StartsWith(line, "import ")) continue;

        int length = (int)strlen(line);

        if (used + length + 1 >= outSize) return;

        memcpy(out + used, line, length);
        used += length;
        out[used++] = '\n';
        out[used] = '\0';
    }
}

// Primeira passada: so cria a classe. Os relacionamentos citam as outras por
// nome, entao nenhum corpo pode ser lido antes de todas existirem.
static bool ReadDeclaration(const char *path, int layoutIndex)
{
    char *text = LoadFileText((char *)path);
    if (text == NULL) return false;

    Rectangle bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    float userWidth = 0.0f;
    float userHeight = 0.0f;
    bool positioned = false;
    bool created = false;
    bool inComment = false;

    const char *cursor = text;
    char raw[LINE_CAPACITY];

    while ((cursor = NextLine(cursor, raw, sizeof(raw))) != NULL)
    {
        char line[LINE_CAPACITY];
        TrimSpaces(raw, line, sizeof(line));

        if (inComment)
        {
            if (strstr(line, "*/") != NULL) inComment = false;
            continue;
        }

        if (StartsWith(line, "/*"))
        {
            if (strstr(line, "*/") == NULL) inComment = true;
            continue;
        }

        const char *marker = strstr(line, MARKER_POSITION);

        if (marker != NULL)
        {
            if (sscanf(marker + strlen(MARKER_POSITION), "%f %f %f %f",
                       &bounds.x, &bounds.y, &userWidth, &userHeight) == 4) positioned = true;

            continue;
        }

        char name[CLASS_NAME_LEN];
        ClassKind kind;

        if (!ParseTypeDeclaration(line, name, sizeof(name), &kind)) continue;

        if (!positioned)
        {
            bounds.x = LAYOUT_ORIGIN + (layoutIndex % LAYOUT_COLUMNS) * LAYOUT_STEP_X;
            bounds.y = LAYOUT_ORIGIN + (layoutIndex / LAYOUT_COLUMNS) * LAYOUT_STEP_Y;
        }

        // Largura e altura reais saem do conteudo no proximo frame, igual ao
        // carregamento de .ruml
        AddClassFromData(GetClassCount() + 1, name, kind, bounds, userWidth, userHeight);
        created = true;

        break;
    }

    UnloadFileText(text);

    return created;
}

static void ReadBody(const char *path)
{
    char *text = LoadFileText((char *)path);
    if (text == NULL) return;

    int classIndex = -1;
    ClassKind kind = CLASS_KIND_CLASS;
    int depth = 0;
    bool inComment = false;
    bool skipNext = false;

    const char *cursor = text;
    char raw[LINE_CAPACITY];

    while ((cursor = NextLine(cursor, raw, sizeof(raw))) != NULL)
    {
        char line[LINE_CAPACITY];
        TrimSpaces(raw, line, sizeof(line));

        if (inComment)
        {
            if (strstr(line, "*/") != NULL) inComment = false;
            continue;
        }

        if (StartsWith(line, "/*"))
        {
            if (strstr(line, "*/") == NULL) inComment = true;
            continue;
        }

        if (depth == 0 && classIndex == -1)
        {
            char name[CLASS_NAME_LEN];
            ClassKind parsed;

            if (ParseTypeDeclaration(line, name, sizeof(name), &parsed))
            {
                classIndex = FindClassIndexByName(name);
                kind = parsed;

                if (classIndex != -1) ParseInheritance(line, GetClass(classIndex)->id);
            }
        }
        else if (depth == 1 && classIndex != -1)
        {
            ParseMemberLine(classIndex, kind, line, &skipNext);
        }

        depth += CountBraces(line);
    }

    UnloadFileText(text);
}

bool ImportJavaCode(const char *package, int *outClassCount, char *outFolder, int outFolderSize)
{
    char cleaned[PACKAGE_LEN] = {0};

    CleanPackageName(package, cleaned, sizeof(cleaned));
    BuildPackageFolder(cleaned, outFolder, outFolderSize);

    return ImportJavaCodeFromFolder(outFolder, outClassCount);
}

bool ImportJavaCodeFromFolder(const char *folder, int *outClassCount)
{
    *outClassCount = 0;

    if (!DirectoryExists(folder)) return false;

    FilePathList files = LoadDirectoryFiles(folder);
    int candidates = 0;

    for (unsigned int i = 0; i < files.count; i++)
    {
        if (IsFileExtension(files.paths[i], ".java")) candidates++;
    }

    // Descartar o diagrama antes de saber se ha o que ler apagaria o trabalho
    // do usuario por causa de uma pasta vazia
    if (candidates == 0)
    {
        UnloadDirectoryFiles(files);
        return false;
    }

    ClearAllClasses();
    ClearAllRelations();

    for (unsigned int i = 0; i < files.count; i++)
    {
        if (!IsFileExtension(files.paths[i], ".java")) continue;

        if (ReadDeclaration(files.paths[i], *outClassCount)) (*outClassCount)++;
    }

    for (unsigned int i = 0; i < files.count; i++)
    {
        if (!IsFileExtension(files.paths[i], ".java")) continue;

        ReadBody(files.paths[i]);
    }

    UnloadDirectoryFiles(files);

    return (*outClassCount) > 0;
}
