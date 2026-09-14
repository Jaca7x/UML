#include "tests.h"
#include "../src/include/validation.h"
#include "../src/include/editor.h"
#include "../src/include/relations.h"
#include <stdio.h>
#include <string.h>

// Quantos problemas de cada gravidade a validacao acha no diagrama atual
static int CountBySeverity(IssueSeverity severity)
{
    int count = 0;

    ValidateDiagram();

    for (int i = 0; i < GetIssueCount(); i++)
    {
        if (GetIssue(i)->severity == severity) count++;
    }

    return count;
}

static bool HasIssueMentioning(const char *piece)
{
    for (int i = 0; i < GetIssueCount(); i++)
    {
        if (strstr(GetIssue(i)->message, piece) != NULL) return true;
    }

    return false;
}

static int AddClass(int id, const char *name, ClassKind kind)
{
    Rectangle spot = {0.0f, 0.0f, 0.0f, 0.0f};

    return AddClassFromData(id, name, kind, spot, 0.0f, 0.0f);
}

static void Reset(void)
{
    ClearAllClasses();
    ClearAllRelations();
}

// Se o diagrama de referencia acusasse algo, as regras estariam gritando com
// quem nao fez nada de errado — e o painel viraria ruido que se aprende a
// ignorar
static void TestCleanDiagramIsSilent(void)
{
    BuildSampleDiagram();

    CHECK_INT(ValidateDiagram(), 0);
    CHECK_INT(GetErrorCount(), 0);
}

// O erro que motivou a validacao: a seta de heranca aponta do filho para o
// pai, e desenhar ao contrario gera "class Animal extends Cachorro" calado
static void TestInvertedInheritance(void)
{
    Reset();

    AddClass(1, "Animal", CLASS_KIND_ABSTRACT);
    AddClass(2, "Cachorro", CLASS_KIND_CLASS);
    AddRelationFromData(1, 2, RELATION_INHERITANCE, "", "");

    CHECK(CountBySeverity(ISSUE_WARNING) >= 1);
    CHECK(HasIssueMentioning("invertida"));

    // No sentido certo, nada a dizer sobre a direcao
    Reset();

    AddClass(1, "Animal", CLASS_KIND_ABSTRACT);
    AddClass(2, "Cachorro", CLASS_KIND_CLASS);
    AddRelationFromData(2, 1, RELATION_INHERITANCE, "", "");

    ValidateDiagram();
    CHECK(!HasIssueMentioning("invertida"));
}

static void TestMultiplicityOnHierarchy(void)
{
    Reset();

    AddClass(1, "Base", CLASS_KIND_CLASS);
    AddClass(2, "Derivada", CLASS_KIND_CLASS);
    AddRelationFromData(2, 1, RELATION_INHERITANCE, "1", "*");

    ValidateDiagram();
    CHECK(HasIssueMentioning("multiplicidade"));

    // Em agregacao a multiplicidade e justamente o que faz sentido
    Reset();

    AddClass(1, "Time", CLASS_KIND_CLASS);
    AddClass(2, "Jogador", CLASS_KIND_CLASS);
    AddRelationFromData(1, 2, RELATION_AGGREGATION, "1", "*");

    ValidateDiagram();
    CHECK(!HasIssueMentioning("multiplicidade"));
}

// Duas classes com o mesmo nome viram o mesmo arquivo .java: a segunda passa
// por cima da primeira sem aviso nenhum
static void TestDuplicateClassName(void)
{
    Reset();

    AddClass(1, "Pedido", CLASS_KIND_CLASS);
    AddClass(2, "Pedido", CLASS_KIND_CLASS);

    CHECK(CountBySeverity(ISSUE_ERROR) >= 1);
    CHECK(HasIssueMentioning("Duas classes"));
}

static void TestInvalidJavaNames(void)
{
    Reset();

    AddClass(1, "Minha Classe", CLASS_KIND_CLASS);   // espaco
    AddClass(2, "class", CLASS_KIND_CLASS);          // palavra reservada
    AddClass(3, "2Rapido", CLASS_KIND_CLASS);        // comeca com digito
    AddClass(4, "Correta", CLASS_KIND_CLASS);

    CHECK_INT(CountBySeverity(ISSUE_ERROR), 3);
    CHECK(HasIssueMentioning("Minha Classe"));
    CHECK(HasIssueMentioning("2Rapido"));
    CHECK(!HasIssueMentioning("\"Correta\""));
}

// Atributo sem tipo vira "private void x;", que nao compila
static void TestAttributeWithoutType(void)
{
    Reset();

    int conta = AddClass(1, "Conta", CLASS_KIND_CLASS);
    AddParamToClass(conta, '-', "saldo", "");

    CHECK(CountBySeverity(ISSUE_ERROR) >= 1);
    CHECK(HasIssueMentioning("sem tipo"));

    // Valor de enum nao tem tipo por natureza, e nao pode ser acusado
    Reset();

    int status = AddClass(1, "Status", CLASS_KIND_ENUM);
    AddParamToClass(status, '+', "ATIVO", "");

    ValidateDiagram();
    CHECK(!HasIssueMentioning("sem tipo"));
}

// Mesmo nome com argumentos diferentes e sobrecarga, que Java aceita
static void TestOverloadIsAllowed(void)
{
    Reset();

    int calc = AddClass(1, "Calculadora", CLASS_KIND_CLASS);
    AddMethodToClass(calc, '+', "somar", "int a, int b", "int");
    AddMethodToClass(calc, '+', "somar", "double a, double b", "double");

    CHECK_INT(CountBySeverity(ISSUE_ERROR), 0);

    // Assinatura inteira repetida, ai sim, nao compila
    AddMethodToClass(calc, '+', "somar", "int a, int b", "int");

    CHECK(CountBySeverity(ISSUE_ERROR) >= 1);
    CHECK(HasIssueMentioning("iguais"));
}

static void TestKindRules(void)
{
    Reset();

    int pagavel = AddClass(1, "Pagavel", CLASS_KIND_INTERFACE);
    AddMethodToClass(pagavel, '+', "pagar", "", "void");
    AddParamToClass(pagavel, '-', "taxa", "double");

    ValidateDiagram();
    CHECK(HasIssueMentioning("vira constante"));

    Reset();

    int status = AddClass(1, "Status", CLASS_KIND_ENUM);
    AddParamToClass(status, '+', "ATIVO", "");
    AddMethodToClass(status, '+', "descrever", "", "String");

    ValidateDiagram();
    CHECK(HasIssueMentioning("nao entram no codigo gerado"));

    Reset();

    AddClass(1, "Forma", CLASS_KIND_ABSTRACT);

    ValidateDiagram();
    CHECK(HasIssueMentioning("podia ser classe comum"));
}

static void TestEnumCannotInherit(void)
{
    Reset();

    AddClass(1, "Status", CLASS_KIND_ENUM);
    AddClass(2, "Base", CLASS_KIND_CLASS);
    AddRelationFromData(1, 2, RELATION_INHERITANCE, "", "");

    CHECK(CountBySeverity(ISSUE_ERROR) >= 1);
    CHECK(HasIssueMentioning("enum nao participa"));
}

// Cada problema aponta a classe que o clique no painel deve selecionar
static void TestIssuesPointAtAClass(void)
{
    Reset();

    AddClass(7, "class", CLASS_KIND_CLASS);

    ValidateDiagram();

    CHECK(GetIssueCount() >= 1);

    if (GetIssueCount() >= 1) CHECK_INT(GetIssue(0)->classId, 7);
}

static void TestEmptyDiagramIsSilent(void)
{
    Reset();

    CHECK_INT(ValidateDiagram(), 0);
}

void RunValidationTests(void)
{
    StartSuite("validation — regras do modelo");

    TestCleanDiagramIsSilent();
    TestInvertedInheritance();
    TestMultiplicityOnHierarchy();
    TestDuplicateClassName();
    TestInvalidJavaNames();
    TestAttributeWithoutType();
    TestOverloadIsAllowed();
    TestKindRules();
    TestEnumCannotInherit();
    TestIssuesPointAtAClass();
    TestEmptyDiagramIsSilent();
}
