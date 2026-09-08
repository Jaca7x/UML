#include "../include/history.h"
#include "../include/storage.h"
#include <stdlib.h>
#include <string.h>

#define MAX_STEPS 64

static char *steps[MAX_STEPS] = {0};
static int stepCount = 0;

void PushHistory(void)
{
    char *snapshot = SerializeDiagram();
    if (snapshot == NULL) return;

    // Estado igual ao ultimo passo nao vira passo novo. E o que evita lixo no
    // historico quando a acao nao mudou nada (clicar numa classe sem arrastar).
    if (stepCount > 0 && strcmp(steps[stepCount - 1], snapshot) == 0)
    {
        free(snapshot);
        return;
    }

    if (stepCount == MAX_STEPS)
    {
        free(steps[0]);
        memmove(steps, steps + 1, (MAX_STEPS - 1) * sizeof(char *));
        stepCount--;
    }

    steps[stepCount] = snapshot;
    stepCount++;
}

void UndoHistory(void)
{
    if (stepCount == 0) return;

    stepCount--;
    DeserializeDiagram(steps[stepCount]);

    free(steps[stepCount]);
    steps[stepCount] = NULL;
}

bool CanUndo(void)
{
    return stepCount > 0;
}
