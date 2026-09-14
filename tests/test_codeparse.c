#include "tests.h"
#include "../src/include/codegen.h"
#include "../src/include/codeparse.h"
#include "../src/include/editor.h"
#include "../src/include/relations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int FindClassByName(const char *name)
{
    for (int i = 0; i < GetClassCount(); i++)
    {
        if (strcmp(GetClass(i)->name, name) == 0) return i;
    }

    return -1;
}

// O teste que justifica os marcadores: sem eles a posicao, a multiplicidade,
// a diferenca entre agregacao e composicao e a dependencia inteira sumiriam a
// cada ida e volta.
static void TestRoundTripIsLossless(void)
{
    BuildSampleDiagram();

    char *before = DumpDiagramByName();
    int count = 0;
    char folder[256] = {0};

    CHECK(GenerateJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder)));
    CHECK(ImportJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder)));

    char *after = DumpDiagramByName();

    CHECK(SameDiagramDump(before, after));

    free(before);
    free(after);
}

// Duas voltas seguidas: se alguma regra acrescentasse membro por conta
// propria, a segunda volta e que denunciaria
static void TestTwoRoundTripsAreStable(void)
{
    BuildSampleDiagram();

    int count = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder));
    ImportJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder));

    char *first = DumpDiagramByName();

    GenerateJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder));
    ImportJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder));

    char *second = DumpDiagramByName();

    CHECK(SameDiagramDump(first, second));

    free(first);
    free(second);
}

// O gerador acrescenta os metodos da interface como stub @Override. Le-los de
// volta como membros proprios inflaria a classe a cada ciclo.
static void TestOverrideStubsAreNotImported(void)
{
    BuildSampleDiagram();

    int count = 0;
    char folder[256] = {0};

    GenerateJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder));
    ImportJavaCode(TEST_PACKAGE, &count, folder, sizeof(folder));

    int cachorro = FindClassByName("Cachorro");

    CHECK(cachorro != -1);

    if (cachorro != -1)
    {
        // So o latir() proprio; o pagar() veio do stub e nao conta
        CHECK_INT(GetClass(cachorro)->methodCount, 1);
        CHECK_STR(GetClass(cachorro)->methods[0].name, "latir");
    }
}

static void WriteJava(const char *folder, const char *name, const char *source)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.java", folder, name);

    SaveFileText(path, (char *)source);
}

// Java escrito a mao nao tem marcador nenhum: o leitor precisa deduzir a
// estrutura do proprio codigo
static void TestPlainJavaIsUnderstood(void)
{
    const char *folder = TEST_WORK_DIR "/java-a-mao";

    if (!DirectoryExists(folder)) MakeDirectory(folder);

    WriteJava(folder, "Veiculo",
        "package oficina;\n"
        "\n"
        "public abstract class Veiculo {\n"
        "    protected String placa;\n"
        "    private int ano;\n"
        "\n"
        "    public Veiculo(String placa) {\n"
        "        this.placa = placa;\n"
        "    }\n"
        "\n"
        "    public abstract void ligar();\n"
        "\n"
        "    public boolean estaNovo() {\n"
        "        return ano > 2020;\n"
        "    }\n"
        "}\n");

    WriteJava(folder, "Carro",
        "package oficina;\n"
        "\n"
        "public class Carro extends Veiculo implements Alugavel {\n"
        "    private Motorista motorista;\n"
        "\n"
        "    public void ligar() {\n"
        "    }\n"
        "}\n");

    WriteJava(folder, "Alugavel",
        "package oficina;\n"
        "\n"
        "public interface Alugavel {\n"
        "    double valorDiaria();\n"
        "}\n");

    WriteJava(folder, "Motorista",
        "package oficina;\n"
        "\n"
        "import java.util.List;\n"
        "\n"
        "public class Motorista {\n"
        "    private String nome;\n"
        "    private List<Carro> carros;\n"
        "}\n");

    ClearAllClasses();
    ClearAllRelations();

    int count = 0;

    CHECK(ImportJavaCodeFromFolder(folder, &count));
    CHECK_INT(count, 4);

    int veiculo = FindClassByName("Veiculo");
    int carro = FindClassByName("Carro");
    int alugavel = FindClassByName("Alugavel");
    int motorista = FindClassByName("Motorista");

    CHECK(veiculo != -1 && carro != -1 && alugavel != -1 && motorista != -1);

    if (veiculo == -1 || carro == -1 || alugavel == -1 || motorista == -1) return;

    CHECK_INT((int)GetClass(veiculo)->kind, (int)CLASS_KIND_ABSTRACT);
    CHECK_INT((int)GetClass(alugavel)->kind, (int)CLASS_KIND_INTERFACE);
    CHECK_INT((int)GetClass(carro)->kind, (int)CLASS_KIND_CLASS);

    // Construtor nao existe no modelo do diagrama e nao pode virar metodo
    CHECK_INT(GetClass(veiculo)->methodCount, 2);
    CHECK_INT(GetClass(veiculo)->paramCount, 2);
    CHECK_STR(GetClass(veiculo)->params[0].name, "placa");

    // boolean volta como bool, que e a forma da paleta do editor
    CHECK_STR(GetClass(veiculo)->methods[1].returnType, "bool");

    // Campo de tipo conhecido e relacionamento, nao atributo
    CHECK_INT(GetClass(carro)->paramCount, 0);
    CHECK_INT(GetClass(motorista)->paramCount, 1);

    // heranca + realizacao + associacao (Carro->Motorista) + agregacao
    CHECK_INT(GetRelationCount(), 4);

    // Sem marcador de posicao, as caixas entram em grade e nao empilhadas
    CHECK(GetClass(0)->bounds.x != GetClass(1)->bounds.x
       || GetClass(0)->bounds.y != GetClass(1)->bounds.y);
}

// Descartar o diagrama e so depois descobrir que nao havia o que ler
// destruiria o trabalho do usuario por causa de uma pasta vazia
static void TestEmptyFolderKeepsDiagram(void)
{
    BuildSampleDiagram();
    int before = GetClassCount();

    const char *empty = TEST_WORK_DIR "/pasta-vazia";
    if (!DirectoryExists(empty)) MakeDirectory(empty);

    int count = 0;

    CHECK(!ImportJavaCodeFromFolder(empty, &count));
    CHECK_INT(GetClassCount(), before);

    CHECK(!ImportJavaCodeFromFolder(TEST_WORK_DIR "/nao-existe", &count));
    CHECK_INT(GetClassCount(), before);
}

void RunCodeparseTests(void)
{
    StartSuite("codeparse — leitura de Java");

    TestRoundTripIsLossless();
    TestTwoRoundTripsAreStable();
    TestOverrideStubsAreNotImported();
    TestPlainJavaIsUnderstood();
    TestEmptyFolderKeepsDiagram();

    RemoveSampleJavaFiles(CODEGEN_FOLDER "/ruml/autoteste");
}
