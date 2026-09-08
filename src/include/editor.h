#ifndef EDITOR_H
#define EDITOR_H
#include "../../lib/raylib.h"

#define CLASS_NAME_LEN 64
#define PARAM_NAME_LEN 48
#define PARAM_TYPE_LEN 32
#define MAX_PARAMS     16

typedef struct
{
    char visibility;
    char name[PARAM_NAME_LEN];
    char type[PARAM_TYPE_LEN];
}UMLParam;

typedef struct
{

    Rectangle bounds;

    int id;

    char name[CLASS_NAME_LEN];
    UMLParam params[MAX_PARAMS];
    int paramCount;
    char methods[256];

    // Tamanho definido pelo usuario nas alcas; o conteudo ainda define o minimo
    float userWidth;
    float userHeight;

    bool isDragging;
    Vector2 dragOffSet;
}UMLClass;

void ArmClassPlacement(void);
bool IsPlacingClass(void);
void UpdateAndDrawBoxes(Camera2D camera, int *cursor);

bool HasSelectedClass(void);
void ClearClassSelection(void);
void DrawClassProperties(Rectangle area, int *cursor);

// Ha uma classe sem nome: o resto do programa fica bloqueado ate preencher
bool IsClassNameRequired(void);

// Alinhamento a grade ao arrastar e redimensionar
void ToggleSnapToGrid(void);
bool IsSnapToGridEnabled(void);

int GetClassCount(void);
int GetClassIdByIndex(int index);
int FindClassIndexById(int id);
int GetClassIndexAt(Vector2 worldPos);
Rectangle GetClassBounds(int index);

// Usados na leitura/escrita de arquivo
const UMLClass *GetClass(int index);
void ClearAllClasses(void);
int AddClassFromData(int id, const char *name, Rectangle bounds, float userWidth, float userHeight);
void AddParamToClass(int index, char visibility, const char *name, const char *type);

#endif
