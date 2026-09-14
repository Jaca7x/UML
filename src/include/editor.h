#ifndef EDITOR_H
#define EDITOR_H
#include "../../lib/raylib.h"

#define CLASS_NAME_LEN 64
#define PARAM_NAME_LEN 48
#define PARAM_TYPE_LEN 32
#define MAX_PARAMS     16

#define METHOD_NAME_LEN 48
#define METHOD_ARGS_LEN 64
#define METHOD_TYPE_LEN 32
#define MAX_METHODS     16

typedef struct
{
    char visibility;
    char name[PARAM_NAME_LEN];
    char type[PARAM_TYPE_LEN];
}UMLParam;

typedef enum
{
    CLASS_KIND_CLASS = 0,
    CLASS_KIND_ABSTRACT,
    CLASS_KIND_INTERFACE,
    CLASS_KIND_ENUM,
    CLASS_KIND_COUNT
}ClassKind;

typedef struct
{
    char visibility;
    char name[METHOD_NAME_LEN];
    char args[METHOD_ARGS_LEN];
    char returnType[METHOD_TYPE_LEN];
}UMLMethod;

typedef struct
{

    Rectangle bounds;

    int id;

    char name[CLASS_NAME_LEN];
    ClassKind kind;
    UMLParam params[MAX_PARAMS];
    int paramCount;
    UMLMethod methods[MAX_METHODS];
    int methodCount;

    // Tamanho definido pelo usuario nas alcas; o conteudo ainda define o minimo
    float userWidth;
    float userHeight;

    bool isSelected;
    bool isDragging;
    Vector2 dragOffSet;
}UMLClass;

void ArmClassPlacement(void);
bool IsPlacingClass(void);
void UpdateAndDrawBoxes(Camera2D camera, int *cursor);

bool HasSelectedClass(void);
void SelectClassById(int id);
int GetSelectedClassCount(void);
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
ClassKind GetClassKind(int index);

const char *GetClassKindKey(ClassKind kind);
ClassKind ParseClassKindKey(const char *key);

// Usados na leitura/escrita de arquivo
const UMLClass *GetClass(int index);
void ClearAllClasses(void);
int AddClassFromData(int id, const char *name, ClassKind kind, Rectangle bounds, float userWidth, float userHeight);
void AddParamToClass(int index, char visibility, const char *name, const char *type);
void AddMethodToClass(int index, char visibility, const char *name, const char *args, const char *returnType);

#endif
