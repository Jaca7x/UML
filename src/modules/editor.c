#include "../include/editor.h"
#include "../include/ui.h"
#include "../include/relations.h"
#include "../include/uifont.h"
#include "../include/widgets.h"
#include "../include/history.h"
#include "../include/theme.h"
#include "../include/renderer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define NAME_FONT_SIZE   16
#define NAME_PADDING     10
#define BASE_BOX_WIDTH  150
#define MAX_BOX_WIDTH   300
#define NAME_LINE_HEIGHT 20
#define STEREOTYPE_FONT       12
#define STEREOTYPE_LINE_HEIGHT 16

#define PARAM_FONT_SIZE     14
#define PARAM_LINE_HEIGHT   18
#define PARAMS_PADDING       8
#define BASE_SECTION_HEIGHT 40

#define RESIZE_HANDLE_SIZE  10
#define TEXT_BUFFER_LEN    128

#define PANEL_FIELD_HEIGHT 26
#define PANEL_ROW_HEIGHT   22
#define PANEL_LABEL_FONT   13
#define PANEL_GAP           6
#define SECTION_SPACING    24
#define BLOCK_INDENT       14
#define PARAM_TYPE_COUNT   10
#define TYPE_COLUMNS        3

typedef enum
{
    FOCUS_NONE = 0,
    FOCUS_CLASS_NAME,
    FOCUS_PARAM_NAME,
    FOCUS_PARAM_TYPE,
    FOCUS_METHOD_NAME,
    FOCUS_METHOD_ARGS,
    FOCUS_METHOD_TYPE
}PanelFocus;

static const char *paramTypes[PARAM_TYPE_COUNT] = {
    "int", "long", "float", "double", "bool",
    "char", "String", "Date", "List", "void"
};

// Chaves gravadas no arquivo: reordenar o enum nao pode invalidar diagramas salvos
static const char *classKindKeys[CLASS_KIND_COUNT] = {"classe", "abstrata", "interface", "enum"};
static const char *classKindLabels[CLASS_KIND_COUNT] = {"Classe", "Abstrata", "Interface", "Enum"};

static const char visibilityChars[3] = {'-', '+', '#'};
static const char *visibilityLabels[3] = {"- priv", "+ pub", "# prot"};

static int clickCount = 0;
static UMLClass *arrayClass = NULL;
static bool isPlacingClass = false;
static int nextClassId = 1;

static int selectedIndex = -1;
static int selectedParam = -1;
static int selectedMethod = -1;
static PanelFocus panelFocus = FOCUS_NONE;

// Laco de selecao (Shift + arrastar no vazio)
static bool marqueeActive = false;
static Vector2 marqueeStart = {0};

static bool snapToGrid = true;

void ToggleSnapToGrid(void)
{
    snapToGrid = !snapToGrid;
}

bool IsSnapToGridEnabled(void)
{
    return snapToGrid;
}

static float SnapValue(float value)
{
    if (!snapToGrid) return value;

    return roundf(value / WORLD_GRID_SPACING) * WORLD_GRID_SPACING;
}

static int resizingIndex = -1;
static int resizingHandle = -1;
static Rectangle resizeStartBounds = {0};
static Vector2 resizeStartMouse = {0};

static void DeselectAll(void);

void ClearClassSelection(void)
{
    DeselectAll();
    selectedIndex = -1;
    selectedParam = -1;
    selectedMethod = -1;
    panelFocus = FOCUS_NONE;
}

// Usado pelo painel de problemas: clicar num aviso leva a classe citada
void SelectClassById(int id)
{
    int index = FindClassIndexById(id);
    if (index == -1) return;

    ClearClassSelection();

    arrayClass[index].isSelected = true;
    selectedIndex = index;
}

bool HasSelectedClass(void)
{
    return selectedIndex != -1;
}

static int FindClassWithEmptyName(void)
{
    for (int i = 0; i < clickCount; i++)
    {
        if (arrayClass[i].name[0] == '\0') return i;
    }

    return -1;
}

bool IsClassNameRequired(void)
{
    return FindClassWithEmptyName() != -1;
}

// Prende a selecao e o foco na classe sem nome, para o campo do painel ser a
// unica coisa com que o usuario consegue interagir.
static void FocusClassMissingName(void)
{
    int index = FindClassWithEmptyName();
    if (index == -1) return;

    if (selectedIndex == index && panelFocus == FOCUS_CLASS_NAME) return;

    selectedIndex = index;
    selectedParam = -1;
    panelFocus = FOCUS_CLASS_NAME;
    ClearRelationSelection();
}

int GetSelectedClassCount(void)
{
    int total = 0;

    for (int i = 0; i < clickCount; i++)
    {
        if (arrayClass[i].isSelected) total++;
    }

    return total;
}

static void DeselectAll(void)
{
    for (int i = 0; i < clickCount; i++) arrayClass[i].isSelected = false;
}

static void SelectClass(int index)
{
    if (selectedIndex != index)
    {
        selectedParam = -1;
        selectedMethod = -1;
        panelFocus = FOCUS_NONE;
    }

    DeselectAll();
    arrayClass[index].isSelected = true;
    selectedIndex = index;
    ClearRelationSelection();
}

// Ctrl+clique soma ou tira do grupo sem perder o resto da selecao
static void ToggleClassInSelection(int index)
{
    arrayClass[index].isSelected = !arrayClass[index].isSelected;
    selectedParam = -1;
    selectedMethod = -1;
    panelFocus = FOCUS_NONE;

    if (arrayClass[index].isSelected)
    {
        selectedIndex = index;
    }
    else if (selectedIndex == index)
    {
        selectedIndex = -1;

        for (int i = 0; i < clickCount; i++)
        {
            if (arrayClass[i].isSelected) selectedIndex = i;
        }
    }

    ClearRelationSelection();
}

void ArmClassPlacement(void)
{
    isPlacingClass = !isPlacingClass;

    if (isPlacingClass)
    {
        ClearClassSelection();
        CancelRelationMode();
    }
}

bool IsPlacingClass(void)
{
    return isPlacingClass;
}

int GetClassCount(void)
{
    return clickCount;
}

int GetClassIdByIndex(int index)
{
    return arrayClass[index].id;
}

int FindClassIndexById(int id)
{
    for (int i = 0; i < clickCount; i++)
    {
        if (arrayClass[i].id == id) return i;
    }

    return -1;
}

int GetClassIndexAt(Vector2 worldPos)
{
    for (int i = clickCount - 1; i >= 0; i--)
    {
        if (CheckCollisionPointRec(worldPos, arrayClass[i].bounds)) return i;
    }

    return -1;
}

Rectangle GetClassBounds(int index)
{
    return arrayClass[index].bounds;
}

ClassKind GetClassKind(int index)
{
    return arrayClass[index].kind;
}

const char *GetClassKindKey(ClassKind kind)
{
    return classKindKeys[kind];
}

ClassKind ParseClassKindKey(const char *key)
{
    for (int i = 0; i < CLASS_KIND_COUNT; i++)
    {
        if (strcmp(key, classKindKeys[i]) == 0) return (ClassKind)i;
    }

    return CLASS_KIND_CLASS;
}

// Linha do estereotipo acima do nome. A UML permite {abstract} como
// alternativa ao nome em italico, que nao temos: a fonte carregada nao tem
// variante italica.
static const char *GetStereotype(const UMLClass *cls)
{
    if (cls->kind == CLASS_KIND_INTERFACE) return "\xc2\xab" "interface" "\xc2\xbb";
    if (cls->kind == CLASS_KIND_ENUM) return "\xc2\xab" "enum" "\xc2\xbb";
    if (cls->kind == CLASS_KIND_ABSTRACT) return "{abstract}";

    return NULL;
}

const UMLClass *GetClass(int index)
{
    return &arrayClass[index];
}

void ClearAllClasses(void)
{
    free(arrayClass);
    arrayClass = NULL;
    clickCount = 0;
    nextClassId = 1;

    ClearClassSelection();
}

int AddClassFromData(int id, const char *name, ClassKind kind, Rectangle bounds, float userWidth, float userHeight)
{
    clickCount++;
    arrayClass = (UMLClass *)realloc(arrayClass, clickCount * sizeof(UMLClass));

    UMLClass *loaded = &arrayClass[clickCount - 1];
    loaded->id = id;
    loaded->kind = kind;
    loaded->bounds = bounds;
    loaded->userWidth = userWidth;
    loaded->userHeight = userHeight;
    loaded->isSelected = false;
    loaded->isDragging = false;
    loaded->paramCount = 0;
    loaded->methodCount = 0;
    snprintf(loaded->name, CLASS_NAME_LEN, "%s", name);

    // Ids do arquivo nao podem colidir com os das proximas classes criadas
    if (id >= nextClassId) nextClassId = id + 1;

    return clickCount - 1;
}

void AddParamToClass(int index, char visibility, const char *name, const char *type)
{
    UMLClass *cls = &arrayClass[index];
    if (cls->paramCount >= MAX_PARAMS) return;

    UMLParam *param = &cls->params[cls->paramCount];
    param->visibility = visibility;
    snprintf(param->name, PARAM_NAME_LEN, "%s", name);
    snprintf(param->type, PARAM_TYPE_LEN, "%s", type);

    cls->paramCount++;
}

static void AddUMLClass(Vector2 position)
{
    clickCount++;
    arrayClass = (UMLClass *)realloc(arrayClass, clickCount * sizeof(UMLClass));

    UMLClass *newClass = &arrayClass[clickCount - 1];
    newClass->bounds.width = BASE_BOX_WIDTH;
    newClass->bounds.height = BASE_SECTION_HEIGHT;
    newClass->bounds.x = SnapValue(position.x - newClass->bounds.width / 2.0f);
    newClass->bounds.y = SnapValue(position.y - newClass->bounds.height / 2.0f);

    newClass->id = nextClassId++;
    newClass->kind = CLASS_KIND_CLASS;
    newClass->isSelected = false;
    newClass->isDragging = false;
    newClass->userWidth = 0.0f;
    newClass->userHeight = 0.0f;
    newClass->paramCount = 0;
    newClass->methodCount = 0;
    snprintf(newClass->name, sizeof(newClass->name), "Classe%d", clickCount);
}

static void RemoveUMLClass(int index)
{
    for (int i = index; i < clickCount - 1; i++)
    {
        arrayClass[i] = arrayClass[i + 1];
    }

    clickCount--;

    if (clickCount > 0)
    {
        arrayClass = (UMLClass *)realloc(arrayClass, clickCount * sizeof(UMLClass));
    }
    else
    {
        free(arrayClass);
        arrayClass = NULL;
    }
}

static void DuplicateUMLClass(int index)
{
    UMLClass copy = arrayClass[index];
    copy.bounds.x += 30;
    copy.bounds.y += 30;
    copy.isDragging = false;

    clickCount++;
    arrayClass = (UMLClass *)realloc(arrayClass, clickCount * sizeof(UMLClass));
    arrayClass[clickCount - 1] = copy;
    arrayClass[clickCount - 1].id = nextClassId++;
    snprintf(arrayClass[clickCount - 1].name, sizeof(arrayClass[clickCount - 1].name), "%s_copia", copy.name);

    SelectClass(clickCount - 1);
}

static void AddParam(int classIndex)
{
    UMLClass *cls = &arrayClass[classIndex];
    if (cls->paramCount >= MAX_PARAMS) return;

    UMLParam *param = &cls->params[cls->paramCount];
    param->visibility = '-';
    snprintf(param->name, PARAM_NAME_LEN, "novoParam");
    snprintf(param->type, PARAM_TYPE_LEN, "int");

    cls->paramCount++;
    selectedParam = cls->paramCount - 1;
    selectedMethod = -1;
    panelFocus = FOCUS_PARAM_NAME;
}

static void DuplicateParam(int classIndex, int paramIndex)
{
    UMLClass *cls = &arrayClass[classIndex];
    if (cls->paramCount >= MAX_PARAMS) return;

    cls->params[cls->paramCount] = cls->params[paramIndex];
    cls->paramCount++;

    selectedParam = cls->paramCount - 1;
    selectedMethod = -1;
    panelFocus = FOCUS_PARAM_NAME;
}

static void RemoveParam(int classIndex, int paramIndex)
{
    UMLClass *cls = &arrayClass[classIndex];

    for (int i = paramIndex; i < cls->paramCount - 1; i++)
    {
        cls->params[i] = cls->params[i + 1];
    }

    cls->paramCount--;

    if (selectedParam >= cls->paramCount) selectedParam = -1;
}

void AddMethodToClass(int index, char visibility, const char *name, const char *args, const char *returnType)
{
    UMLClass *cls = &arrayClass[index];
    if (cls->methodCount >= MAX_METHODS) return;

    UMLMethod *method = &cls->methods[cls->methodCount];
    method->visibility = visibility;
    snprintf(method->name, METHOD_NAME_LEN, "%s", name);
    snprintf(method->args, METHOD_ARGS_LEN, "%s", args);
    snprintf(method->returnType, METHOD_TYPE_LEN, "%s", returnType);

    cls->methodCount++;
}

static void AddMethod(int classIndex)
{
    UMLClass *cls = &arrayClass[classIndex];
    if (cls->methodCount >= MAX_METHODS) return;

    AddMethodToClass(classIndex, '+', "novoMetodo", "", "void");

    selectedMethod = cls->methodCount - 1;
    selectedParam = -1;
    panelFocus = FOCUS_METHOD_NAME;
}

static void DuplicateMethod(int classIndex, int methodIndex)
{
    UMLClass *cls = &arrayClass[classIndex];
    if (cls->methodCount >= MAX_METHODS) return;

    cls->methods[cls->methodCount] = cls->methods[methodIndex];
    cls->methodCount++;

    selectedMethod = cls->methodCount - 1;
    selectedParam = -1;
    panelFocus = FOCUS_METHOD_NAME;
}

static void RemoveMethod(int classIndex, int methodIndex)
{
    UMLClass *cls = &arrayClass[classIndex];

    for (int i = methodIndex; i < cls->methodCount - 1; i++)
    {
        cls->methods[i] = cls->methods[i + 1];
    }

    cls->methodCount--;

    if (selectedMethod >= cls->methodCount) selectedMethod = -1;
}

// Assinatura UML: visibilidade nome(argumentos): retorno
static void FormatMethod(const UMLMethod *method, char *out, int outSize)
{
    snprintf(out, outSize, "%c %s(%s): %s", method->visibility, method->name, method->args, method->returnType);
}

static void FormatParam(const UMLParam *param, char *out, int outSize)
{
    snprintf(out, outSize, "%c %s: %s", param->visibility, param->name, param->type);
}

static int WrapText(const char *text, int fontSize, int maxTextWidth, char *out, int outSize)
{
    int textLen = (int)strlen(text);
    int outLen = 0;
    int lineChars = 0;
    int lines = 1;
    char lineBuf[TEXT_BUFFER_LEN + 8];

    for (int i = 0; i < textLen && outLen < outSize - 2; i++)
    {
        memcpy(lineBuf, &text[i - lineChars], lineChars);
        lineBuf[lineChars] = text[i];
        lineBuf[lineChars + 1] = '\0';

        if (lineChars > 0 && MeasureUiText(lineBuf, fontSize) > maxTextWidth)
        {
            out[outLen++] = '\n';
            lines++;
            lineChars = 0;
        }

        out[outLen++] = text[i];
        lineChars++;
    }
    out[outLen] = '\0';

    return lines;
}

static int GetWrappedText(const char *text, int fontSize, int maxTextWidth, char *out, int outSize)
{
    if (MeasureUiText(text, fontSize) <= maxTextWidth)
    {
        snprintf(out, outSize, "%s", text);
        return 1;
    }

    return WrapText(text, fontSize, maxTextWidth, out, outSize);
}

static int GetTextMaxWidth(void)
{
    return MAX_BOX_WIDTH - NAME_PADDING * 2;
}

static float GetNameSectionHeight(int lines)
{
    return NAME_PADDING + lines * NAME_LINE_HEIGHT + 6;
}

static float GetStereotypeHeight(const UMLClass *cls)
{
    return (GetStereotype(cls) != NULL) ? STEREOTYPE_LINE_HEIGHT : 0.0f;
}

static void GetContentSize(const UMLClass *cls, float *outWidth, float *outHeight)
{
    char buf[TEXT_BUFFER_LEN * 2];
    int nameLines = GetWrappedText(cls->name, NAME_FONT_SIZE, GetTextMaxWidth(), buf, sizeof(buf));
    int width = MeasureUiText(cls->name, NAME_FONT_SIZE) + NAME_PADDING * 2;

    const char *stereotype = GetStereotype(cls);
    if (stereotype != NULL)
    {
        int stereotypeWidth = MeasureUiText(stereotype, STEREOTYPE_FONT) + NAME_PADDING * 2;
        if (stereotypeWidth > width) width = stereotypeWidth;
    }

    float paramsHeight = PARAMS_PADDING * 2;

    for (int i = 0; i < cls->paramCount; i++)
    {
        char text[TEXT_BUFFER_LEN];
        FormatParam(&cls->params[i], text, sizeof(text));

        paramsHeight += GetWrappedText(text, PARAM_FONT_SIZE, GetTextMaxWidth(), buf, sizeof(buf)) * PARAM_LINE_HEIGHT;

        int paramWidth = MeasureUiText(text, PARAM_FONT_SIZE) + NAME_PADDING * 2;
        if (paramWidth > width) width = paramWidth;
    }

    float methodsHeight = PARAMS_PADDING * 2;

    for (int i = 0; i < cls->methodCount; i++)
    {
        char text[TEXT_BUFFER_LEN];
        FormatMethod(&cls->methods[i], text, sizeof(text));

        methodsHeight += GetWrappedText(text, PARAM_FONT_SIZE, GetTextMaxWidth(), buf, sizeof(buf)) * PARAM_LINE_HEIGHT;

        int methodWidth = MeasureUiText(text, PARAM_FONT_SIZE) + NAME_PADDING * 2;
        if (methodWidth > width) width = methodWidth;
    }

    if (paramsHeight < BASE_SECTION_HEIGHT) paramsHeight = BASE_SECTION_HEIGHT;
    if (methodsHeight < BASE_SECTION_HEIGHT) methodsHeight = BASE_SECTION_HEIGHT;
    if (width < BASE_BOX_WIDTH) width = BASE_BOX_WIDTH;
    if (width > MAX_BOX_WIDTH) width = MAX_BOX_WIDTH;

    *outWidth = (float)width;
    *outHeight = GetStereotypeHeight(cls) + GetNameSectionHeight(nameLines) + paramsHeight + methodsHeight;
}

static void UpdateClassBoxSize(UMLClass *cls)
{
    float contentWidth, contentHeight;
    GetContentSize(cls, &contentWidth, &contentHeight);

    cls->bounds.width = (cls->userWidth > contentWidth) ? cls->userWidth : contentWidth;
    cls->bounds.height = (cls->userHeight > contentHeight) ? cls->userHeight : contentHeight;
}

// Cantos: 0 = superior esquerdo, 1 = superior direito, 2 = inferior direito, 3 = inferior esquerdo
static Rectangle GetResizeHandle(Rectangle box, int corner)
{
    float half = RESIZE_HANDLE_SIZE / 2.0f;
    float x = (corner == 1 || corner == 2) ? box.x + box.width : box.x;
    float y = (corner == 2 || corner == 3) ? box.y + box.height : box.y;

    return (Rectangle){x - half, y - half, RESIZE_HANDLE_SIZE, RESIZE_HANDLE_SIZE};
}

static void ApplyResize(Vector2 mousePos)
{
    UMLClass *cls = &arrayClass[resizingIndex];
    float contentWidth, contentHeight;
    GetContentSize(cls, &contentWidth, &contentHeight);

    float left = resizeStartBounds.x;
    float top = resizeStartBounds.y;
    float right = resizeStartBounds.x + resizeStartBounds.width;
    float bottom = resizeStartBounds.y + resizeStartBounds.height;

    float dx = mousePos.x - resizeStartMouse.x;
    float dy = mousePos.y - resizeStartMouse.y;

    if (resizingHandle == 0 || resizingHandle == 3)
    {
        left = SnapValue(left + dx);
        if (right - left < contentWidth) left = right - contentWidth;
    }
    else
    {
        right = SnapValue(right + dx);
        if (right - left < contentWidth) right = left + contentWidth;
    }

    if (resizingHandle == 0 || resizingHandle == 1)
    {
        top = SnapValue(top + dy);
        if (bottom - top < contentHeight) top = bottom - contentHeight;
    }
    else
    {
        bottom = SnapValue(bottom + dy);
        if (bottom - top < contentHeight) bottom = top + contentHeight;
    }

    cls->bounds.x = left;
    cls->bounds.y = top;
    cls->userWidth = right - left;
    cls->userHeight = bottom - top;
}

static bool HandleResize(Vector2 mousePos, int *cursor)
{
    if (resizingIndex != -1)
    {
        *cursor = (resizingHandle == 0 || resizingHandle == 2) ? MOUSE_CURSOR_RESIZE_NWSE : MOUSE_CURSOR_RESIZE_NESW;
        ApplyResize(mousePos);

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) resizingIndex = -1;

        return true;
    }

    if (selectedIndex == -1 || GetSelectedClassCount() > 1) return false;

    for (int corner = 0; corner < 4; corner++)
    {
        if (!CheckCollisionPointRec(mousePos, GetResizeHandle(arrayClass[selectedIndex].bounds, corner))) continue;

        *cursor = (corner == 0 || corner == 2) ? MOUSE_CURSOR_RESIZE_NWSE : MOUSE_CURSOR_RESIZE_NESW;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            PushHistory();
            resizingIndex = selectedIndex;
            resizingHandle = corner;
            resizeStartBounds = arrayClass[selectedIndex].bounds;
            resizeStartMouse = mousePos;
        }

        return true;
    }

    return false;
}

void UpdateAndDrawBoxes(Camera2D camera, int *cursor) {
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    bool nameRequired = IsClassNameRequired();

    if (nameRequired) FocusClassMissingName();

    bool blocked = nameRequired || IsMouseOverUi() || IsRelationModeArmed()
                || IsConnectingRelation() || DidRelationsConsumeClick();

    for (int i = 0; i < clickCount; i++)
    {
        UpdateClassBoxSize(&arrayClass[i]);
    }

    // Backspace/Delete so apagam a classe fora de um campo de texto, senao
    // apagariam a classe enquanto o usuario corrige um nome
    if (!nameRequired && selectedIndex != -1 && panelFocus == FOCUS_NONE
        && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE)))
    {
        PushHistory();

        for (int i = clickCount - 1; i >= 0; i--)
        {
            if (arrayClass[i].isSelected) RemoveUMLClass(i);
        }

        ClearClassSelection();
        return;
    }

    if (marqueeActive)
    {
        *cursor = MOUSE_CURSOR_CROSSHAIR;

        Rectangle marquee = {
            (mousePos.x < marqueeStart.x) ? mousePos.x : marqueeStart.x,
            (mousePos.y < marqueeStart.y) ? mousePos.y : marqueeStart.y,
            fabsf(mousePos.x - marqueeStart.x),
            fabsf(mousePos.y - marqueeStart.y)
        };

        DrawRectangleRec(marquee, Fade(ThemeAccent(), 0.15f));
        DrawRectangleLinesEx(marquee, 1, ThemeAccent());

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            DeselectAll();
            selectedIndex = -1;

            for (int i = 0; i < clickCount; i++)
            {
                if (!CheckCollisionRecs(marquee, arrayClass[i].bounds)) continue;

                arrayClass[i].isSelected = true;
                selectedIndex = i;
            }

            if (selectedIndex != -1) ClearRelationSelection();
            marqueeActive = false;
        }
    }
    else if (!blocked && !HandleResize(mousePos, cursor))
    {
        if (isPlacingClass)
        {
            *cursor = MOUSE_CURSOR_CROSSHAIR;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                PushHistory();
                AddUMLClass(mousePos);
            }
        }
        else
        {
            int hovered = GetClassIndexAt(mousePos);

            if (hovered != -1) *cursor = MOUSE_CURSOR_POINTING_HAND;

            bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (hovered == -1)
                {
                    if (shiftDown)
                    {
                        marqueeActive = true;
                        marqueeStart = mousePos;
                    }
                    else
                    {
                        ClearClassSelection();
                    }
                }
                else if (ctrlDown)
                {
                    PushHistory();
                    ToggleClassInSelection(hovered);
                }
                else
                {
                    PushHistory();

                    // Clicar numa classe ja selecionada mantem o grupo: e assim
                    // que se arrasta varias de uma vez
                    if (!arrayClass[hovered].isSelected) SelectClass(hovered);
                    else selectedIndex = hovered;

                    for (int i = 0; i < clickCount; i++)
                    {
                        if (!arrayClass[i].isSelected) continue;

                        arrayClass[i].isDragging = true;
                        arrayClass[i].dragOffSet.x = mousePos.x - arrayClass[i].bounds.x;
                        arrayClass[i].dragOffSet.y = mousePos.y - arrayClass[i].bounds.y;
                    }
                }
            }
        }
    }

    for (int i = 0; i < clickCount; i++) {
        Color colorClass = arrayClass[i].isSelected ? ThemeAccent() : ThemeBorder();

        if (arrayClass[i].isDragging)
        {
            *cursor = MOUSE_CURSOR_RESIZE_ALL;

            arrayClass[i].bounds.x = SnapValue(mousePos.x - arrayClass[i].dragOffSet.x);
            arrayClass[i].bounds.y = SnapValue(mousePos.y - arrayClass[i].dragOffSet.y);
        }


        char display[TEXT_BUFFER_LEN * 2];
        int nameLines = GetWrappedText(arrayClass[i].name, NAME_FONT_SIZE, GetTextMaxWidth(), display, sizeof(display));

        float stereotypeHeight = GetStereotypeHeight(&arrayClass[i]);
        float dividerY = arrayClass[i].bounds.y + stereotypeHeight + GetNameSectionHeight(nameLines);

        DrawRectangleLinesEx(arrayClass[i].bounds, 2, colorClass);

        const char *stereotype = GetStereotype(&arrayClass[i]);
        if (stereotype != NULL)
        {
            int stereotypeWidth = MeasureUiText(stereotype, STEREOTYPE_FONT);
            DrawUiText(stereotype,
                       arrayClass[i].bounds.x + (arrayClass[i].bounds.width - stereotypeWidth) / 2.0f,
                       arrayClass[i].bounds.y + 6, STEREOTYPE_FONT, ThemeTextMuted());
        }

        DrawUiText(display, arrayClass[i].bounds.x + NAME_PADDING,
                   arrayClass[i].bounds.y + stereotypeHeight + NAME_PADDING, NAME_FONT_SIZE, ThemeText());
        DrawLine(arrayClass[i].bounds.x, dividerY, arrayClass[i].bounds.x + arrayClass[i].bounds.width, dividerY, colorClass);

        float rowY = dividerY + PARAMS_PADDING;
        float paramsBottom = dividerY;

        for (int p = 0; p < arrayClass[i].paramCount; p++)
        {
            char text[TEXT_BUFFER_LEN];
            FormatParam(&arrayClass[i].params[p], text, sizeof(text));

            int lines = GetWrappedText(text, PARAM_FONT_SIZE, GetTextMaxWidth(), display, sizeof(display));
            Color rowColor = (i == selectedIndex && p == selectedParam) ? ThemeAccent() : ThemeTextMuted();

            DrawUiText(display, arrayClass[i].bounds.x + NAME_PADDING, rowY, PARAM_FONT_SIZE, rowColor);
            rowY += lines * PARAM_LINE_HEIGHT;
        }

        // Segunda divisoria: atributos em cima, operacoes embaixo
        float paramsUsed = rowY - dividerY - PARAMS_PADDING + PARAMS_PADDING * 2;
        paramsBottom = dividerY + ((paramsUsed < BASE_SECTION_HEIGHT) ? BASE_SECTION_HEIGHT : paramsUsed);

        DrawLine(arrayClass[i].bounds.x, paramsBottom,
                 arrayClass[i].bounds.x + arrayClass[i].bounds.width, paramsBottom, colorClass);

        rowY = paramsBottom + PARAMS_PADDING;

        for (int m = 0; m < arrayClass[i].methodCount; m++)
        {
            char text[TEXT_BUFFER_LEN];
            FormatMethod(&arrayClass[i].methods[m], text, sizeof(text));

            int lines = GetWrappedText(text, PARAM_FONT_SIZE, GetTextMaxWidth(), display, sizeof(display));
            Color rowColor = (i == selectedIndex && m == selectedMethod) ? ThemeAccent() : ThemeTextMuted();

            DrawUiText(display, arrayClass[i].bounds.x + NAME_PADDING, rowY, PARAM_FONT_SIZE, rowColor);
            rowY += lines * PARAM_LINE_HEIGHT;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        for (int i = 0; i < clickCount; i++) arrayClass[i].isDragging = false;
    }

    if (selectedIndex != -1 && GetSelectedClassCount() == 1)
    {
        for (int corner = 0; corner < 4; corner++)
        {
            Rectangle handle = GetResizeHandle(arrayClass[selectedIndex].bounds, corner);
            DrawRectangleRec(handle, ThemeSurface());
            DrawRectangleLinesEx(handle, 2, ThemeAccent());
        }
    }
}



// O painel da classe e dividido por entidade: cada aba concentra a lista e o
// formulario do seu proprio item, em vez de tudo competir pela mesma coluna.
static int classPanelTab = 0;

static void DrawMemberTypeGrid(Rectangle block, float y, char *target, int capacity, int *cursor)
{
    float typeWidth = (block.width - PANEL_GAP * (TYPE_COLUMNS - 1)) / TYPE_COLUMNS;

    for (int i = 0; i < PARAM_TYPE_COUNT; i++)
    {
        Rectangle rect = {block.x + (i % TYPE_COLUMNS) * (typeWidth + PANEL_GAP),
                          y + (i / TYPE_COLUMNS) * (PANEL_ROW_HEIGHT + PANEL_GAP),
                          typeWidth, PANEL_ROW_HEIGHT};

        if (PanelButton(rect, paramTypes[i], strcmp(target, paramTypes[i]) == 0, ThemeAccent(), 11, cursor))
        {
            PushHistory();
            snprintf(target, capacity, "%s", paramTypes[i]);
        }
    }
}

static void DrawVisibilityRow(Rectangle block, float y, char *visibility, int *cursor)
{
    float width = (block.width - PANEL_GAP * 2) / 3.0f;

    for (int i = 0; i < 3; i++)
    {
        Rectangle rect = {block.x + i * (width + PANEL_GAP), y, width, PANEL_ROW_HEIGHT};

        if (PanelButton(rect, visibilityLabels[i], *visibility == visibilityChars[i], ThemeAccent(), 12, cursor))
        {
            PushHistory();
            *visibility = visibilityChars[i];
        }
    }
}

// Cabecalho e botao de adicionar, comuns as abas de atributo e metodo
static float DrawListHeader(Rectangle area, float y, const char *label, bool canAdd,
                            int *cursor, void (*onAdd)(int), int classIndex)
{
    DrawUiText(label, area.x, y, 12, ThemeAccent());

    Rectangle addBtn = {area.x + area.width - 96, y - 6, 96, PANEL_ROW_HEIGHT};

    if (canAdd)
    {
        if (PanelButton(addBtn, "+ Adicionar", false, ThemeAccent(), 12, cursor))
        {
            PushHistory();
            onAdd(classIndex);
        }
    }
    else
    {
        DrawUiText("limite atingido", addBtn.x, addBtn.y + 5, 11, ThemeDanger());
    }

    return y + PANEL_ROW_HEIGHT + PANEL_GAP;
}

static void DrawClassTab(Rectangle area, UMLClass *cls, int *cursor)
{
    float y = area.y;

    PanelLabel("Nome da classe", area.x, y);
    y += 18;

    if (PanelField((Rectangle){area.x, y, area.width, PANEL_FIELD_HEIGHT}, cls->name,
                   panelFocus == FOCUS_CLASS_NAME, cursor))
    {
        if (panelFocus != FOCUS_CLASS_NAME) PushHistory();
        panelFocus = FOCUS_CLASS_NAME;
    }
    y += PANEL_FIELD_HEIGHT + 8;

    if (cls->name[0] == '\0')
    {
        DrawUiText("A classe precisa de um nome", area.x, y, 12, ThemeDanger());
    }
    y += 26;

    PanelLabel("Tipo", area.x, y);
    y += 18;

    float kindWidth = (area.width - PANEL_GAP) / 2.0f;
    for (int i = 0; i < CLASS_KIND_COUNT; i++)
    {
        Rectangle rect = {area.x + (i % 2) * (kindWidth + PANEL_GAP),
                          y + (i / 2) * (PANEL_ROW_HEIGHT + PANEL_GAP),
                          kindWidth, PANEL_ROW_HEIGHT};

        if (PanelButton(rect, classKindLabels[i], cls->kind == (ClassKind)i, ThemeAccent(), 12, cursor))
        {
            PushHistory();
            cls->kind = (ClassKind)i;
        }
    }
    y += 2 * (PANEL_ROW_HEIGHT + PANEL_GAP) + 12;

    char summary[96];
    snprintf(summary, sizeof(summary), "%d atributo(s) e %d metodo(s)", cls->paramCount, cls->methodCount);
    DrawUiText(summary, area.x, y, 12, ThemeTextMuted());
}

static void DrawParamsTab(Rectangle area, UMLClass *cls, int *cursor)
{
    float y = area.y;

    y = DrawListHeader(area, y, (cls->kind == CLASS_KIND_ENUM) ? "VALORES" : "ATRIBUTOS",
                       cls->paramCount < MAX_PARAMS, cursor, AddParam, selectedIndex);

    for (int i = 0; i < cls->paramCount; i++)
    {
        Rectangle row = {area.x, y, area.width - 52, PANEL_ROW_HEIGHT};
        Rectangle copyBtn = {area.x + area.width - 50, y, 24, PANEL_ROW_HEIGHT};
        Rectangle removeBtn = {area.x + area.width - 24, y, 24, PANEL_ROW_HEIGHT};

        char text[TEXT_BUFFER_LEN];
        FormatParam(&cls->params[i], text, sizeof(text));

        bool selected = (i == selectedParam);
        bool hover = CheckCollisionPointRec(GetMousePosition(), row);
        if (hover) *cursor = MOUSE_CURSOR_POINTING_HAND;

        DrawRectangleRec(row, selected ? Fade(ThemeAccent(), 0.20f) : ThemeSurface());
        DrawRectangleLinesEx(row, 1, selected ? ThemeAccent() : ThemeBorder());
        DrawUiText(text, row.x + 6, row.y + 5, 12, ThemeText());

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (selectedParam != i) PushHistory();
            selectedParam = i;
            panelFocus = FOCUS_PARAM_NAME;
        }

        if (PanelButton(copyBtn, "++", false, ThemeAccent(), 11, cursor))
        {
            PushHistory();
            DuplicateParam(selectedIndex, i);
            return;
        }

        if (PanelButton(removeBtn, "x", false, ThemeDanger(), 12, cursor))
        {
            PushHistory();
            RemoveParam(selectedIndex, i);
            return;
        }

        y += PANEL_ROW_HEIGHT + 4;
    }

    if (selectedParam == -1 || selectedParam >= cls->paramCount) return;

    UMLParam *param = &cls->params[selectedParam];
    Rectangle block = {area.x + BLOCK_INDENT, area.y, area.width - BLOCK_INDENT, area.height};

    y += SECTION_SPACING;
    DrawLine(area.x, y, area.x + area.width, y, ThemeBorder());
    y += 18;

    char header[96];
    snprintf(header, sizeof(header), "EDITANDO: %s", param->name);
    DrawUiText(header, area.x, y, 12, ThemeAccent());
    y += 26;

    float blockStart = y - 10;

    PanelLabel("Visibilidade", block.x, y);
    y += 18;
    DrawVisibilityRow(block, y, &param->visibility, cursor);
    y += PANEL_ROW_HEIGHT + 16;

    PanelLabel("Nome", block.x, y);
    y += 18;
    if (PanelField((Rectangle){block.x, y, block.width, PANEL_FIELD_HEIGHT}, param->name,
                   panelFocus == FOCUS_PARAM_NAME, cursor))
    {
        if (panelFocus != FOCUS_PARAM_NAME) PushHistory();
        panelFocus = FOCUS_PARAM_NAME;
    }
    y += PANEL_FIELD_HEIGHT + 16;

    PanelLabel("Tipo", block.x, y);
    y += 18;
    if (PanelField((Rectangle){block.x, y, block.width, PANEL_FIELD_HEIGHT}, param->type,
                   panelFocus == FOCUS_PARAM_TYPE, cursor))
    {
        if (panelFocus != FOCUS_PARAM_TYPE) PushHistory();
        panelFocus = FOCUS_PARAM_TYPE;
    }
    y += PANEL_FIELD_HEIGHT + PANEL_GAP;

    DrawMemberTypeGrid(block, y, param->type, PARAM_TYPE_LEN, cursor);

    int rows = (PARAM_TYPE_COUNT + TYPE_COLUMNS - 1) / TYPE_COLUMNS;
    DrawRectangle(area.x, blockStart, 3, y + rows * (PANEL_ROW_HEIGHT + PANEL_GAP) - blockStart,
                  Fade(ThemeAccent(), 0.55f));
}

static void DrawMethodsTab(Rectangle area, UMLClass *cls, int *cursor)
{
    float y = area.y;

    y = DrawListHeader(area, y, "METODOS", cls->methodCount < MAX_METHODS, cursor, AddMethod, selectedIndex);

    for (int i = 0; i < cls->methodCount; i++)
    {
        Rectangle row = {area.x, y, area.width - 52, PANEL_ROW_HEIGHT};
        Rectangle copyBtn = {area.x + area.width - 50, y, 24, PANEL_ROW_HEIGHT};
        Rectangle removeBtn = {area.x + area.width - 24, y, 24, PANEL_ROW_HEIGHT};

        char text[TEXT_BUFFER_LEN];
        FormatMethod(&cls->methods[i], text, sizeof(text));

        bool selected = (i == selectedMethod);
        bool hover = CheckCollisionPointRec(GetMousePosition(), row);
        if (hover) *cursor = MOUSE_CURSOR_POINTING_HAND;

        DrawRectangleRec(row, selected ? Fade(ThemeAccent(), 0.20f) : ThemeSurface());
        DrawRectangleLinesEx(row, 1, selected ? ThemeAccent() : ThemeBorder());
        DrawUiText(text, row.x + 6, row.y + 5, 12, ThemeText());

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (selectedMethod != i) PushHistory();
            selectedMethod = i;
            panelFocus = FOCUS_METHOD_NAME;
        }

        if (PanelButton(copyBtn, "++", false, ThemeAccent(), 11, cursor))
        {
            PushHistory();
            DuplicateMethod(selectedIndex, i);
            return;
        }

        if (PanelButton(removeBtn, "x", false, ThemeDanger(), 12, cursor))
        {
            PushHistory();
            RemoveMethod(selectedIndex, i);
            return;
        }

        y += PANEL_ROW_HEIGHT + 4;
    }

    if (selectedMethod == -1 || selectedMethod >= cls->methodCount) return;

    UMLMethod *method = &cls->methods[selectedMethod];
    Rectangle block = {area.x + BLOCK_INDENT, area.y, area.width - BLOCK_INDENT, area.height};

    y += SECTION_SPACING;
    DrawLine(area.x, y, area.x + area.width, y, ThemeBorder());
    y += 18;

    char header[96];
    snprintf(header, sizeof(header), "EDITANDO: %s", method->name);
    DrawUiText(header, area.x, y, 12, ThemeAccent());
    y += 26;

    float blockStart = y - 10;

    PanelLabel("Visibilidade", block.x, y);
    y += 18;
    DrawVisibilityRow(block, y, &method->visibility, cursor);
    y += PANEL_ROW_HEIGHT + 16;

    PanelLabel("Nome", block.x, y);
    y += 18;
    if (PanelField((Rectangle){block.x, y, block.width, PANEL_FIELD_HEIGHT}, method->name,
                   panelFocus == FOCUS_METHOD_NAME, cursor))
    {
        if (panelFocus != FOCUS_METHOD_NAME) PushHistory();
        panelFocus = FOCUS_METHOD_NAME;
    }
    y += PANEL_FIELD_HEIGHT + 16;

    PanelLabel("Argumentos (ex: valor: double)", block.x, y);
    y += 18;
    if (PanelField((Rectangle){block.x, y, block.width, PANEL_FIELD_HEIGHT}, method->args,
                   panelFocus == FOCUS_METHOD_ARGS, cursor))
    {
        if (panelFocus != FOCUS_METHOD_ARGS) PushHistory();
        panelFocus = FOCUS_METHOD_ARGS;
    }
    y += PANEL_FIELD_HEIGHT + 16;

    PanelLabel("Retorno", block.x, y);
    y += 18;
    if (PanelField((Rectangle){block.x, y, block.width, PANEL_FIELD_HEIGHT}, method->returnType,
                   panelFocus == FOCUS_METHOD_TYPE, cursor))
    {
        if (panelFocus != FOCUS_METHOD_TYPE) PushHistory();
        panelFocus = FOCUS_METHOD_TYPE;
    }
    y += PANEL_FIELD_HEIGHT + PANEL_GAP;

    DrawMemberTypeGrid(block, y, method->returnType, METHOD_TYPE_LEN, cursor);

    int rows = (PARAM_TYPE_COUNT + TYPE_COLUMNS - 1) / TYPE_COLUMNS;
    DrawRectangle(area.x, blockStart, 3, y + rows * (PANEL_ROW_HEIGHT + PANEL_GAP) - blockStart,
                  Fade(ThemeAccent(), 0.55f));
}

void DrawClassProperties(Rectangle area, int *cursor)
{
    if (selectedIndex == -1) return;

    // Com varias selecionadas nao ha "a classe" para editar: o painel vira
    // acao de grupo
    if (GetSelectedClassCount() > 1)
    {
        char summary[64];
        snprintf(summary, sizeof(summary), "%d classes selecionadas", GetSelectedClassCount());
        DrawUiText(summary, area.x, area.y, 14, ThemeText());
        PanelLabel("Arraste qualquer uma para mover o grupo.", area.x, area.y + 24);

        if (PanelButton((Rectangle){area.x, area.y + area.height - 30, area.width, 28},
                        "Excluir selecionadas", false, ThemeDanger(), 13, cursor))
        {
            PushHistory();

            for (int i = clickCount - 1; i >= 0; i--)
            {
                if (arrayClass[i].isSelected) RemoveUMLClass(i);
            }

            ClearClassSelection();
        }

        return;
    }

    UMLClass *cls = &arrayClass[selectedIndex];

    if (IsFileFieldFocused()) panelFocus = FOCUS_NONE;

    if (panelFocus == FOCUS_CLASS_NAME) AppendTypedChars(cls->name, CLASS_NAME_LEN);
    else if (panelFocus == FOCUS_PARAM_NAME && selectedParam != -1) AppendTypedChars(cls->params[selectedParam].name, PARAM_NAME_LEN);
    else if (panelFocus == FOCUS_PARAM_TYPE && selectedParam != -1) AppendTypedChars(cls->params[selectedParam].type, PARAM_TYPE_LEN);
    else if (panelFocus == FOCUS_METHOD_NAME && selectedMethod != -1) AppendTypedChars(cls->methods[selectedMethod].name, METHOD_NAME_LEN);
    else if (panelFocus == FOCUS_METHOD_ARGS && selectedMethod != -1) AppendTypedChars(cls->methods[selectedMethod].args, METHOD_ARGS_LEN);
    else if (panelFocus == FOCUS_METHOD_TYPE && selectedMethod != -1) AppendTypedChars(cls->methods[selectedMethod].returnType, METHOD_TYPE_LEN);

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) panelFocus = FOCUS_NONE;

    // Classe sem nome prende o painel na aba onde o nome se edita
    if (cls->name[0] == '\0') classPanelTab = 0;

    const char *tabNames[] = {"Classe", "Atributos", "Metodos"};
    classPanelTab = PanelTabs((Rectangle){area.x, area.y, area.width, 24}, tabNames, 3, classPanelTab, cursor);

    Rectangle content = {area.x, area.y + 24 + SECTION_SPACING, area.width, area.height - 24 - SECTION_SPACING - 40};

    if (classPanelTab == 0) DrawClassTab(content, cls, cursor);
    else if (classPanelTab == 1) DrawParamsTab(content, cls, cursor);
    else DrawMethodsTab(content, cls, cursor);

    float buttonsY = area.y + area.height - 30;
    float buttonWidth = (area.width - PANEL_GAP) / 2.0f;

    if (PanelButton((Rectangle){area.x, buttonsY, buttonWidth, 28}, "Duplicar", false, ThemeAccent(), 13, cursor))
    {
        PushHistory();
        DuplicateUMLClass(selectedIndex);
        return;
    }

    if (PanelButton((Rectangle){area.x + buttonWidth + PANEL_GAP, buttonsY, buttonWidth, 28},
                    "Excluir", false, ThemeDanger(), 13, cursor))
    {
        PushHistory();
        RemoveUMLClass(selectedIndex);
        ClearClassSelection();
    }
}
