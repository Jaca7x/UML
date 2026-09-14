#include "../include/validation.h"
#include "../include/editor.h"
#include "../include/relations.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Enquanto o diagrama era so um desenho, um erro nele era um desenho errado.
// Com a geracao de codigo ele virou entrada de compilador: heranca desenhada
// ao contrario vira "class Animal extends Cachorro", em silencio.
//
// Estas regras existem para o sistema dizer em voz alta o que antes so
// aparecia quando alguem tentava compilar o resultado.
//
// A divisao e por consequencia, nao por gravidade sentida:
//   ERRO   o codigo gerado nao compila, ou o arquivo nao faz sentido
//   AVISO  compila, mas provavelmente nao e o que se quis dizer

static ValidationIssue issues[MAX_ISSUES];
static int issueCount = 0;
static int errorCount = 0;

int GetIssueCount(void)
{
    return issueCount;
}

int GetErrorCount(void)
{
    return errorCount;
}

const ValidationIssue *GetIssue(int index)
{
    if (index < 0 || index >= issueCount) return NULL;

    return &issues[index];
}

static void AddIssue(IssueSeverity severity, int classId, const char *format, ...)
{
    if (issueCount >= MAX_ISSUES) return;

    ValidationIssue *issue = &issues[issueCount++];

    issue->severity = severity;
    issue->classId = classId;

    va_list args;
    va_start(args, format);
    vsnprintf(issue->message, sizeof(issue->message), format, args);
    va_end(args);

    if (severity == ISSUE_ERROR) errorCount++;
}

// Palavras que o compilador Java ja usa: como nome de classe, de atributo ou
// de metodo, qualquer uma delas produz arquivo que nao compila.
static const char *javaKeywords[] = {
    "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
    "class", "const", "continue", "default", "do", "double", "else", "enum",
    "extends", "final", "finally", "float", "for", "goto", "if", "implements",
    "import", "instanceof", "int", "interface", "long", "native", "new",
    "package", "private", "protected", "public", "return", "short", "static",
    "strictfp", "super", "switch", "synchronized", "this", "throw", "throws",
    "transient", "try", "void", "volatile", "while", "true", "false", "null"
};

static bool IsJavaKeyword(const char *name)
{
    int count = sizeof(javaKeywords) / sizeof(javaKeywords[0]);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(name, javaKeywords[i]) == 0) return true;
    }

    return false;
}

static bool IsLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

// Identificador Java: comeca por letra, _ ou $, e segue com letra, digito, _
// ou $. Espaco e acento, que o editor aceita digitar, nao passam.
static bool IsValidIdentifier(const char *name)
{
    if (name[0] == '\0') return false;
    if (!IsLetter(name[0]) && name[0] != '_' && name[0] != '$') return false;

    for (int i = 1; name[i] != '\0'; i++)
    {
        if (!IsLetter(name[i]) && !IsDigit(name[i]) && name[i] != '_' && name[i] != '$') return false;
    }

    return !IsJavaKeyword(name);
}

static const char *KindLabel(ClassKind kind)
{
    if (kind == CLASS_KIND_INTERFACE) return "interface";
    if (kind == CLASS_KIND_ENUM) return "enum";
    if (kind == CLASS_KIND_ABSTRACT) return "abstrata";

    return "classe";
}

static void CheckNames(void)
{
    for (int i = 0; i < GetClassCount(); i++)
    {
        const UMLClass *cls = GetClass(i);

        if (cls->name[0] == '\0')
        {
            AddIssue(ISSUE_ERROR, cls->id, "Ha uma classe sem nome");
            continue;
        }

        if (!IsValidIdentifier(cls->name))
        {
            AddIssue(ISSUE_ERROR, cls->id,
                     "\"%s\" nao serve como nome de classe em Java", cls->name);
        }

        // Duas classes com o mesmo nome viram o mesmo arquivo: a segunda
        // geracao passa por cima da primeira
        for (int other = i + 1; other < GetClassCount(); other++)
        {
            if (strcmp(cls->name, GetClass(other)->name) != 0) continue;

            AddIssue(ISSUE_ERROR, cls->id, "Duas classes chamadas \"%s\"", cls->name);
            break;
        }
    }
}

static void CheckMembers(const UMLClass *cls)
{
    for (int p = 0; p < cls->paramCount; p++)
    {
        const UMLParam *param = &cls->params[p];

        if (param->name[0] == '\0')
        {
            AddIssue(ISSUE_ERROR, cls->id, "%s: ha um atributo sem nome", cls->name);
            continue;
        }

        if (!IsValidIdentifier(param->name))
        {
            AddIssue(ISSUE_ERROR, cls->id, "%s: \"%s\" nao serve como nome de atributo",
                     cls->name, param->name);
        }

        // Valor de enum nao tem tipo; atributo sem tipo vira "private void x;",
        // que nao compila
        if (cls->kind != CLASS_KIND_ENUM && param->type[0] == '\0')
        {
            AddIssue(ISSUE_ERROR, cls->id, "%s: o atributo \"%s\" esta sem tipo",
                     cls->name, param->name);
        }

        for (int other = p + 1; other < cls->paramCount; other++)
        {
            if (strcmp(param->name, cls->params[other].name) != 0) continue;

            AddIssue(ISSUE_ERROR, cls->id, "%s: dois atributos chamados \"%s\"",
                     cls->name, param->name);
            break;
        }
    }

    for (int m = 0; m < cls->methodCount; m++)
    {
        const UMLMethod *method = &cls->methods[m];

        if (method->name[0] == '\0')
        {
            AddIssue(ISSUE_ERROR, cls->id, "%s: ha um metodo sem nome", cls->name);
            continue;
        }

        if (!IsValidIdentifier(method->name))
        {
            AddIssue(ISSUE_ERROR, cls->id, "%s: \"%s\" nao serve como nome de metodo",
                     cls->name, method->name);
        }

        // Mesmo nome com argumentos diferentes e sobrecarga, que e valida.
        // So a assinatura inteira repetida e que nao compila.
        for (int other = m + 1; other < cls->methodCount; other++)
        {
            if (strcmp(method->name, cls->methods[other].name) != 0) continue;
            if (strcmp(method->args, cls->methods[other].args) != 0) continue;

            AddIssue(ISSUE_ERROR, cls->id, "%s: dois metodos \"%s(%s)\" iguais",
                     cls->name, method->name, method->args);
            break;
        }
    }
}

static void CheckKindRules(const UMLClass *cls)
{
    if (cls->kind == CLASS_KIND_INTERFACE)
    {
        if (cls->paramCount > 0)
        {
            AddIssue(ISSUE_WARNING, cls->id,
                     "%s e interface: atributo nela vira constante em Java", cls->name);
        }

        if (cls->methodCount == 0)
        {
            AddIssue(ISSUE_WARNING, cls->id,
                     "%s e interface sem metodo: contrato vazio", cls->name);
        }
    }

    if (cls->kind == CLASS_KIND_ENUM && cls->methodCount > 0)
    {
        AddIssue(ISSUE_WARNING, cls->id,
                 "%s e enum: os metodos nao entram no codigo gerado", cls->name);
    }

    if (cls->kind == CLASS_KIND_ENUM && cls->paramCount == 0)
    {
        AddIssue(ISSUE_WARNING, cls->id, "%s e enum sem nenhum valor", cls->name);
    }

    if (cls->kind == CLASS_KIND_ABSTRACT && cls->methodCount == 0)
    {
        AddIssue(ISSUE_WARNING, cls->id,
                 "%s e abstrata mas nao tem metodo: podia ser classe comum", cls->name);
    }
}

static void CheckRelations(void)
{
    for (int i = 0; i < GetRelationCount(); i++)
    {
        const UMLRelation *relation = GetRelation(i);

        int fromIndex = FindClassIndexById(relation->fromId);
        int toIndex = FindClassIndexById(relation->toId);

        if (fromIndex == -1 || toIndex == -1) continue;

        const UMLClass *from = GetClass(fromIndex);
        const UMLClass *to = GetClass(toIndex);

        bool isHierarchy = (relation->type == RELATION_INHERITANCE
                         || relation->type == RELATION_REALIZATION);

        // Heranca nao tem "quantos": a multiplicidade so faz sentido em
        // associacao, agregacao e composicao
        if (isHierarchy && (relation->fromMultiplicity[0] != '\0'
                         || relation->toMultiplicity[0] != '\0'))
        {
            AddIssue(ISSUE_WARNING, from->id,
                     "%s -> %s: %s nao tem multiplicidade", from->name, to->name,
                     GetRelationTypeKey(relation->type));
        }

        if (relation->type != RELATION_INHERITANCE) continue;

        // A seta de heranca aponta do filho para o pai. Abstrata herdando de
        // concreta quase sempre significa que ela foi desenhada ao contrario.
        if (from->kind == CLASS_KIND_ABSTRACT && to->kind == CLASS_KIND_CLASS)
        {
            AddIssue(ISSUE_WARNING, from->id,
                     "%s (abstrata) herda de %s (concreta): a seta esta invertida?",
                     from->name, to->name);
        }

        // Interface nao herda de classe, e enum nao herda de nada
        if (from->kind == CLASS_KIND_ENUM || to->kind == CLASS_KIND_ENUM)
        {
            AddIssue(ISSUE_ERROR, from->id,
                     "%s -> %s: enum nao participa de heranca", from->name, to->name);
        }

        if (from->kind == CLASS_KIND_INTERFACE && to->kind != CLASS_KIND_INTERFACE)
        {
            AddIssue(ISSUE_ERROR, from->id,
                     "%s e interface e so pode herdar de outra interface, nao de %s (%s)",
                     from->name, to->name, KindLabel(to->kind));
        }
    }
}

int ValidateDiagram(void)
{
    issueCount = 0;
    errorCount = 0;

    CheckNames();

    for (int i = 0; i < GetClassCount(); i++)
    {
        const UMLClass *cls = GetClass(i);

        CheckMembers(cls);
        CheckKindRules(cls);
    }

    CheckRelations();

    return issueCount;
}
