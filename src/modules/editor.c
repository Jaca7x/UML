#include "../include/editor.h"
#include "../include/ui.h"
#include "../include/relations.h"
#include "../include/uifont.h"
#include "../include/widgets.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NAME_FONT_SIZE   16
#define NAME_PADDING     10
#define BASE_BOX_WIDTH  150
#define MAX_BOX_WIDTH   300
#define NAME_LINE_HEIGHT 20

#define PARAM_FONT_SIZE     14
#define PARAM_LINE_HEIGHT   18
#define PARAMS_PADDING       8
#define BASE_PARAMS_HEIGHT  64

#define RESIZE_HANDLE_SIZE  10
#define TEXT_BUFFER_LEN    128

#define PANEL_FIELD_HEIGHT 26
#define PANEL_ROW_HEIGHT   22
#define PANEL_LABEL_FONT   13
#define PANEL_GAP           6
#define PARAM_TYPE_COUNT   10
#define TYPE_COLUMNS        3

typedef enum
{
    FOCUS_NONE = 0,
    FOCUS_CLASS_NAME,
    FOCUS_PARAM_NAME,
    FOCUS_PARAM_TYPE
}PanelFocus;

static const char *paramTypes[PARAM_TYPE_COUNT] = {
    "int", "long", "float", "double", "bool",
    "char", "String", "Date", "List", "void"
};

static const char visibilityChars[3] = {'-', '+', '#'};
static const char *visibilityLabels[3] = {"- priv", "+ pub", "# prot"};

static int clickCount = 0;
static UMLClass *arrayClass = NULL;
static bool isPlacingClass = false;
static int nextClassId = 1;

static int selectedIndex = -1;
static int selectedParam = -1;
static PanelFocus panelFocus = FOCUS_NONE;

static int resizingIndex = -1;
static int resizingHandle = -1;
static Rectangle resizeStartBounds = {0};
static Vector2 resizeStartMouse = {0};

void ClearClassSelection(void)
{
    selectedIndex = -1;
    selectedParam = -1;
    panelFocus = FOCUS_NONE;
}

bool HasSelectedClass(void)
{
    return selectedIndex != -1;
}

static void SelectClass(int index)
{
    if (selectedIndex != index)
    {
        selectedParam = -1;
        panelFocus = FOCUS_NONE;
    }

    selectedIndex = index;
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

int AddClassFromData(int id, const char *name, Rectangle bounds, float userWidth, float userHeight)
{
    clickCount++;
    arrayClass = (UMLClass *)realloc(arrayClass, clickCount * sizeof(UMLClass));

    UMLClass *loaded = &arrayClass[clickCount - 1];
    loaded->id = id;
    loaded->bounds = bounds;
    loaded->userWidth = userWidth;
    loaded->userHeight = userHeight;
    loaded->isDragging = false;
    loaded->paramCount = 0;
    loaded->methods[0] = '\0';
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
    newClass->bounds.height = BASE_PARAMS_HEIGHT;
    newClass->bounds.x = position.x - newClass->bounds.width / 2.0f;
    newClass->bounds.y = position.y - newClass->bounds.height / 2.0f;

    newClass->id = nextClassId++;
    newClass->isDragging = false;
    newClass->userWidth = 0.0f;
    newClass->userHeight = 0.0f;
    newClass->paramCount = 0;
    newClass->methods[0] = '\0';
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

static void GetContentSize(const UMLClass *cls, float *outWidth, float *outHeight)
{
    char buf[TEXT_BUFFER_LEN * 2];
    int nameLines = GetWrappedText(cls->name, NAME_FONT_SIZE, GetTextMaxWidth(), buf, sizeof(buf));
    int width = MeasureUiText(cls->name, NAME_FONT_SIZE) + NAME_PADDING * 2;
    float paramsHeight = PARAMS_PADDING * 2;

    for (int i = 0; i < cls->paramCount; i++)
    {
        char text[TEXT_BUFFER_LEN];
        FormatParam(&cls->params[i], text, sizeof(text));

        paramsHeight += GetWrappedText(text, PARAM_FONT_SIZE, GetTextMaxWidth(), buf, sizeof(buf)) * PARAM_LINE_HEIGHT;

        int paramWidth = MeasureUiText(text, PARAM_FONT_SIZE) + NAME_PADDING * 2;
        if (paramWidth > width) width = paramWidth;
    }

    if (paramsHeight < BASE_PARAMS_HEIGHT) paramsHeight = BASE_PARAMS_HEIGHT;
    if (width < BASE_BOX_WIDTH) width = BASE_BOX_WIDTH;
    if (width > MAX_BOX_WIDTH) width = MAX_BOX_WIDTH;

    *outWidth = (float)width;
    *outHeight = GetNameSectionHeight(nameLines) + paramsHeight;
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
        left += dx;
        if (right - left < contentWidth) left = right - contentWidth;
    }
    else
    {
        right += dx;
        if (right - left < contentWidth) right = left + contentWidth;
    }

    if (resizingHandle == 0 || resizingHandle == 1)
    {
        top += dy;
        if (bottom - top < contentHeight) top = bottom - contentHeight;
    }
    else
    {
        bottom += dy;
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

    if (selectedIndex == -1) return false;

    for (int corner = 0; corner < 4; corner++)
    {
        if (!CheckCollisionPointRec(mousePos, GetResizeHandle(arrayClass[selectedIndex].bounds, corner))) continue;

        *cursor = (corner == 0 || corner == 2) ? MOUSE_CURSOR_RESIZE_NWSE : MOUSE_CURSOR_RESIZE_NESW;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
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
    bool blocked = IsMouseOverUi() || IsRelationModeArmed() || IsConnectingRelation() || DidRelationsConsumeClick();

    for (int i = 0; i < clickCount; i++)
    {
        UpdateClassBoxSize(&arrayClass[i]);
    }

    if (!blocked && !HandleResize(mousePos, cursor))
    {
        if (isPlacingClass)
        {
            *cursor = MOUSE_CURSOR_CROSSHAIR;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) AddUMLClass(mousePos);
        }
        else
        {
            int hovered = GetClassIndexAt(mousePos);

            if (hovered != -1) *cursor = MOUSE_CURSOR_POINTING_HAND;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (hovered == -1)
                {
                    ClearClassSelection();
                }
                else
                {
                    SelectClass(hovered);
                    arrayClass[hovered].isDragging = true;
                    arrayClass[hovered].dragOffSet.x = mousePos.x - arrayClass[hovered].bounds.x;
                    arrayClass[hovered].dragOffSet.y = mousePos.y - arrayClass[hovered].bounds.y;
                }
            }
        }
    }

    for (int i = 0; i < clickCount; i++) {
        Color colorClass = (i == selectedIndex) ? BLUE : BLACK;

        if (arrayClass[i].isDragging)
        {
            *cursor = MOUSE_CURSOR_RESIZE_ALL;

            arrayClass[i].bounds.x = mousePos.x - arrayClass[i].dragOffSet.x;
            arrayClass[i].bounds.y = mousePos.y - arrayClass[i].dragOffSet.y;
        }


        char display[TEXT_BUFFER_LEN * 2];
        int nameLines = GetWrappedText(arrayClass[i].name, NAME_FONT_SIZE, GetTextMaxWidth(), display, sizeof(display));
        float dividerY = arrayClass[i].bounds.y + GetNameSectionHeight(nameLines);

        DrawRectangleLinesEx(arrayClass[i].bounds, 2, colorClass);
        DrawUiText(display, arrayClass[i].bounds.x + NAME_PADDING, arrayClass[i].bounds.y + NAME_PADDING, NAME_FONT_SIZE, BLACK);
        DrawLine(arrayClass[i].bounds.x, dividerY, arrayClass[i].bounds.x + arrayClass[i].bounds.width, dividerY, colorClass);

        float rowY = dividerY + PARAMS_PADDING;

        for (int p = 0; p < arrayClass[i].paramCount; p++)
        {
            char text[TEXT_BUFFER_LEN];
            FormatParam(&arrayClass[i].params[p], text, sizeof(text));

            int lines = GetWrappedText(text, PARAM_FONT_SIZE, GetTextMaxWidth(), display, sizeof(display));
            Color rowColor = (i == selectedIndex && p == selectedParam) ? BLUE : DARKGRAY;

            DrawUiText(display, arrayClass[i].bounds.x + NAME_PADDING, rowY, PARAM_FONT_SIZE, rowColor);
            rowY += lines * PARAM_LINE_HEIGHT;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        for (int i = 0; i < clickCount; i++) arrayClass[i].isDragging = false;
    }

    if (selectedIndex != -1)
    {
        for (int corner = 0; corner < 4; corner++)
        {
            Rectangle handle = GetResizeHandle(arrayClass[selectedIndex].bounds, corner);
            DrawRectangleRec(handle, RAYWHITE);
            DrawRectangleLinesEx(handle, 2, BLUE);
        }
    }
}

void DrawClassProperties(Rectangle area, int *cursor)
{
    if (selectedIndex == -1) return;

    UMLClass *cls = &arrayClass[selectedIndex];
    float y = area.y;

    if (panelFocus == FOCUS_CLASS_NAME) AppendTypedChars(cls->name, CLASS_NAME_LEN);
    else if (panelFocus == FOCUS_PARAM_NAME && selectedParam != -1) AppendTypedChars(cls->params[selectedParam].name, PARAM_NAME_LEN);
    else if (panelFocus == FOCUS_PARAM_TYPE && selectedParam != -1) AppendTypedChars(cls->params[selectedParam].type, PARAM_TYPE_LEN);

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) panelFocus = FOCUS_NONE;

    PanelLabel("Nome da classe", area.x, y);
    y += 18;

    if (PanelField((Rectangle){area.x, y, area.width, PANEL_FIELD_HEIGHT}, cls->name, panelFocus == FOCUS_CLASS_NAME, cursor))
    {
        panelFocus = FOCUS_CLASS_NAME;
    }
    y += PANEL_FIELD_HEIGHT;

    if (cls->name[0] == '\0')
    {
        DrawUiText("A classe precisa de um nome", area.x, y + 4, 12, RED);
    }
    y += 22;

    PanelLabel("Parametros", area.x, y + 6);

    Rectangle addBtn = {area.x + area.width - 90, y, 90, PANEL_ROW_HEIGHT};
    if (cls->paramCount < MAX_PARAMS)
    {
        if (PanelButton(addBtn, "+ Adicionar", false, BLUE, 12, cursor)) AddParam(selectedIndex);
    }
    else
    {
        DrawUiText("limite atingido", addBtn.x, addBtn.y + 5, 11, RED);
    }
    y += PANEL_ROW_HEIGHT + PANEL_GAP;

    for (int i = 0; i < cls->paramCount; i++)
    {
        Rectangle row = {area.x, y, area.width - 26, PANEL_ROW_HEIGHT};
        Rectangle removeBtn = {area.x + area.width - 22, y, 22, PANEL_ROW_HEIGHT};

        char text[TEXT_BUFFER_LEN];
        FormatParam(&cls->params[i], text, sizeof(text));

        bool isSelectedParam = (i == selectedParam);
        bool hover = CheckCollisionPointRec(GetMousePosition(), row);
        if (hover) *cursor = MOUSE_CURSOR_POINTING_HAND;

        DrawRectangleRec(row, isSelectedParam ? Fade(BLUE, 0.15f) : RAYWHITE);
        DrawRectangleLinesEx(row, 1, isSelectedParam ? BLUE : GRAY);
        DrawUiText(text, row.x + 6, row.y + 5, 12, BLACK);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            selectedParam = i;
            panelFocus = FOCUS_PARAM_NAME;
        }

        if (PanelButton(removeBtn, "x", false, RED, 12, cursor))
        {
            RemoveParam(selectedIndex, i);
            return;
        }

        y += PANEL_ROW_HEIGHT + 4;
    }

    if (selectedParam != -1 && selectedParam < cls->paramCount)
    {
        UMLParam *param = &cls->params[selectedParam];

        y += 10;
        DrawLine(area.x, y, area.x + area.width, y, GRAY);
        y += 12;

        PanelLabel("Visibilidade", area.x, y);
        y += 18;

        float visWidth = (area.width - PANEL_GAP * 2) / 3.0f;
        for (int i = 0; i < 3; i++)
        {
            Rectangle rect = {area.x + i * (visWidth + PANEL_GAP), y, visWidth, PANEL_ROW_HEIGHT};
            if (PanelButton(rect, visibilityLabels[i], param->visibility == visibilityChars[i], BLUE, 12, cursor))
            {
                param->visibility = visibilityChars[i];
            }
        }
        y += PANEL_ROW_HEIGHT + 12;

        PanelLabel("Nome", area.x, y);
        y += 18;
        if (PanelField((Rectangle){area.x, y, area.width, PANEL_FIELD_HEIGHT}, param->name, panelFocus == FOCUS_PARAM_NAME, cursor))
        {
            panelFocus = FOCUS_PARAM_NAME;
        }
        y += PANEL_FIELD_HEIGHT + 12;

        PanelLabel("Tipo", area.x, y);
        y += 18;
        if (PanelField((Rectangle){area.x, y, area.width, PANEL_FIELD_HEIGHT}, param->type, panelFocus == FOCUS_PARAM_TYPE, cursor))
        {
            panelFocus = FOCUS_PARAM_TYPE;
        }
        y += PANEL_FIELD_HEIGHT + PANEL_GAP;

        float typeWidth = (area.width - PANEL_GAP * (TYPE_COLUMNS - 1)) / TYPE_COLUMNS;
        for (int i = 0; i < PARAM_TYPE_COUNT; i++)
        {
            Rectangle rect = {area.x + (i % TYPE_COLUMNS) * (typeWidth + PANEL_GAP),
                              y + (i / TYPE_COLUMNS) * (PANEL_ROW_HEIGHT + PANEL_GAP),
                              typeWidth, PANEL_ROW_HEIGHT};

            if (PanelButton(rect, paramTypes[i], strcmp(param->type, paramTypes[i]) == 0, BLUE, 11, cursor))
            {
                snprintf(param->type, PARAM_TYPE_LEN, "%s", paramTypes[i]);
            }
        }
    }

    float buttonsY = area.y + area.height - 30;
    float buttonWidth = (area.width - PANEL_GAP) / 2.0f;

    if (PanelButton((Rectangle){area.x, buttonsY, buttonWidth, 28}, "Duplicar", false, BLUE, 13, cursor))
    {
        DuplicateUMLClass(selectedIndex);
        return;
    }

    if (PanelButton((Rectangle){area.x + buttonWidth + PANEL_GAP, buttonsY, buttonWidth, 28}, "Excluir", false, RED, 13, cursor))
    {
        RemoveUMLClass(selectedIndex);
        ClearClassSelection();
    }
}
