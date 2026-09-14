#ifndef VALIDATION_H
#define VALIDATION_H
#include "../../lib/raylib.h"

#define MAX_ISSUES      64
#define ISSUE_TEXT_LEN 160

typedef enum
{
    // Gera codigo que nao compila, ou arquivo que nao faz sentido
    ISSUE_ERROR = 0,

    // O diagrama funciona, mas quase certamente nao e o que se quis dizer
    ISSUE_WARNING
}IssueSeverity;

typedef struct
{
    IssueSeverity severity;
    char message[ISSUE_TEXT_LEN];

    // Classe a selecionar ao clicar no problema; -1 quando nao ha uma so
    int classId;
}ValidationIssue;

// Percorre o diagrama e monta a lista de problemas. Devolve quantos achou.
int ValidateDiagram(void);

int GetIssueCount(void);
int GetErrorCount(void);
const ValidationIssue *GetIssue(int index);

#endif
