#include "../include/relations.h"
#include "../include/editor.h"
#include "../include/ui.h"
#include "../include/uifont.h"
#include "../include/widgets.h"
#include "../include/history.h"
#include "../include/theme.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define LINE_THICKNESS      2.0f
#define ORNAMENT_LENGTH    14.0f
#define ORNAMENT_HALFWIDTH  7.0f
#define LINE_HIT_DISTANCE   6.0f
#define MULTIPLICITY_FONT  12
#define LABEL_DISTANCE     20.0f

#define CONNECT_HANDLE_SIZE   14.0f
#define CONNECT_HANDLE_OFFSET 16.0f

#define PANEL_FIELD_HEIGHT 26
#define PANEL_ROW_HEIGHT   22
#define PANEL_LABEL_FONT   13
#define PANEL_GAP           6
#define QUICK_COUNT         6

static const char *relationLabels[RELATION_TYPE_COUNT] = {
    "Associacao", "Heranca", "Agregacao", "Composicao", "Dependencia", "Realizacao"
};

static const char *quickMultiplicity[QUICK_COUNT] = {"1", "0..1", "1..*", "0..*", "*", "vazio"};

typedef enum
{
    SIDE_TOP = 0,
    SIDE_RIGHT,
    SIDE_BOTTOM,
    SIDE_LEFT
}BoxSide;

typedef enum
{
    FOCUS_NONE = 0,
    FOCUS_FROM,
    FOCUS_TO
}PanelFocus;

typedef struct
{
    Vector2 start;
    Vector2 end;
    bool valid;
}RelationGeometry;

static UMLRelation *relations = NULL;
static int relationCount = 0;

// Pontas ja resolvidas no frame, mais os buffers de trabalho da distribuicao por borda
static RelationGeometry *geometry = NULL;
static int geometryCapacity = 0;
static int *endpointClass = NULL;
static int *endpointSide = NULL;
static int *sideTotals = NULL;
static int *sideUsed = NULL;
static int sideCapacity = 0;

static bool relationArmed = false;
static int pendingFromId = -1;

static int selectedRelation = -1;
static PanelFocus panelFocus = FOCUS_NONE;
static const char *panelError = NULL;

static int connectFromId = -1;
static Vector2 connectPressPos = {0};
static bool connectMoved = false;
static bool consumedClick = false;

bool IsConnectingRelation(void)
{
    return connectFromId != -1;
}

bool DidRelationsConsumeClick(void)
{
    return consumedClick;
}

bool IsRelationModeArmed(void)
{
    return relationArmed;
}

bool HasSelectedRelation(void)
{
    return selectedRelation != -1;
}

void ClearRelationSelection(void)
{
    selectedRelation = -1;
    panelFocus = FOCUS_NONE;
    panelError = NULL;
}

void CancelRelationMode(void)
{
    relationArmed = false;
    pendingFromId = -1;
}

void ArmRelationMode(void)
{
    relationArmed = !relationArmed;
    pendingFromId = -1;

    if (relationArmed)
    {
        ClearRelationSelection();
        ClearClassSelection();
        if (IsPlacingClass()) ArmClassPlacement();
    }
}

// Chaves gravadas no arquivo. Nao usar o valor numerico do enum: reordenar
// os tipos invalidaria todos os diagramas ja salvos.
static const char *relationKeys[RELATION_TYPE_COUNT] = {
    "associacao", "heranca", "agregacao", "composicao", "dependencia", "realizacao"
};

const char *GetRelationTypeKey(RelationType type)
{
    return relationKeys[type];
}

RelationType ParseRelationTypeKey(const char *key)
{
    for (int i = 0; i < RELATION_TYPE_COUNT; i++)
    {
        if (strcmp(key, relationKeys[i]) == 0) return (RelationType)i;
    }

    return RELATION_ASSOCIATION;
}

int GetRelationCount(void)
{
    return relationCount;
}

const UMLRelation *GetRelation(int index)
{
    return &relations[index];
}

void ClearAllRelations(void)
{
    free(relations);
    relations = NULL;
    relationCount = 0;

    ClearRelationSelection();
}

void AddRelationFromData(int fromId, int toId, RelationType type, const char *fromMultiplicity, const char *toMultiplicity)
{
    relationCount++;
    relations = (UMLRelation *)realloc(relations, relationCount * sizeof(UMLRelation));

    UMLRelation *loaded = &relations[relationCount - 1];
    loaded->fromId = fromId;
    loaded->toId = toId;
    loaded->type = type;
    snprintf(loaded->fromMultiplicity, MULTIPLICITY_LEN, "%s", fromMultiplicity);
    snprintf(loaded->toMultiplicity, MULTIPLICITY_LEN, "%s", toMultiplicity);
}

static void RemoveRelation(int index)
{
    for (int i = index; i < relationCount - 1; i++)
    {
        relations[i] = relations[i + 1];
    }

    relationCount--;

    if (relationCount > 0)
    {
        relations = (UMLRelation *)realloc(relations, relationCount * sizeof(UMLRelation));
    }
    else
    {
        free(relations);
        relations = NULL;
    }
}

// Sobe a cadeia de heranca a partir de childId procurando ancestorId.
// A trava de profundidade so protege contra dados invalidos: ciclos sao barrados na criacao.
static bool InheritsFrom(int childId, int ancestorId, int depth)
{
    if (depth > relationCount) return false;

    for (int i = 0; i < relationCount; i++)
    {
        if (relations[i].type != RELATION_INHERITANCE && relations[i].type != RELATION_REALIZATION) continue;
        if (relations[i].fromId != childId) continue;

        if (relations[i].toId == ancestorId) return true;
        if (InheritsFrom(relations[i].toId, ancestorId, depth + 1)) return true;
    }

    return false;
}

static const char *ValidateRelation(int ignoreIndex, int fromId, int toId, RelationType type)
{
    for (int i = 0; i < relationCount; i++)
    {
        if (i == ignoreIndex) continue;
        if (relations[i].type != type) continue;

        bool sameDirection = (relations[i].fromId == fromId && relations[i].toId == toId);
        bool reversed = (relations[i].fromId == toId && relations[i].toId == fromId);

        // A associacao e uma linha simples, entao invertida fica identica
        if (sameDirection || (reversed && type == RELATION_ASSOCIATION))
        {
            return "Esse relacionamento ja existe";
        }
    }

    if (type == RELATION_INHERITANCE || type == RELATION_REALIZATION)
    {
        if (InheritsFrom(toId, fromId, 0)) return "Isso criaria heranca circular";
    }

    // Realizacao e "implementa interface": o destino precisa ser uma
    if (type == RELATION_REALIZATION)
    {
        int toIndex = FindClassIndexById(toId);

        if (toIndex != -1 && GetClassKind(toIndex) != CLASS_KIND_INTERFACE)
        {
            return "Realizacao exige uma interface no destino";
        }
    }

    return NULL;
}

static void CreateRelation(int fromId, int toId)
{
    panelError = ValidateRelation(-1, fromId, toId, RELATION_ASSOCIATION);
    if (panelError != NULL) return;

    PushHistory();

    relationCount++;
    relations = (UMLRelation *)realloc(relations, relationCount * sizeof(UMLRelation));

    UMLRelation *created = &relations[relationCount - 1];
    created->fromId = fromId;
    created->toId = toId;
    created->type = RELATION_ASSOCIATION;
    created->fromMultiplicity[0] = '\0';
    created->toMultiplicity[0] = '\0';

    ClearClassSelection();
    selectedRelation = relationCount - 1;
    panelFocus = FOCUS_NONE;
}

static void PruneDanglingRelations(void)
{
    for (int i = relationCount - 1; i >= 0; i--)
    {
        if (FindClassIndexById(relations[i].fromId) == -1 || FindClassIndexById(relations[i].toId) == -1)
        {
            RemoveRelation(i);
            if (selectedRelation == i) ClearRelationSelection();
            else if (selectedRelation > i) selectedRelation--;
        }
    }
}

static Vector2 GetRectCenter(Rectangle rect)
{
    return (Vector2){rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

// Qual borda da caixa a linha ate `target` atravessa.
static BoxSide GetExitSide(Rectangle box, Vector2 target)
{
    Vector2 center = GetRectCenter(box);
    float dx = target.x - center.x;
    float dy = target.y - center.y;

    if (fabsf(dx) * box.height >= fabsf(dy) * box.width) return (dx >= 0.0f) ? SIDE_RIGHT : SIDE_LEFT;

    return (dy >= 0.0f) ? SIDE_BOTTOM : SIDE_TOP;
}

// Distribui `total` conexoes igualmente ao longo da borda, sem encostar nos cantos.
static Vector2 GetAnchorOnSide(Rectangle box, BoxSide side, int slot, int total)
{
    float t = (float)(slot + 1) / (float)(total + 1);

    switch (side)
    {
        case SIDE_TOP:    return (Vector2){box.x + box.width * t, box.y};
        case SIDE_BOTTOM: return (Vector2){box.x + box.width * t, box.y + box.height};
        case SIDE_LEFT:   return (Vector2){box.x, box.y + box.height * t};
        default:          return (Vector2){box.x + box.width, box.y + box.height * t};
    }
}

// Conta quantas conexoes saem da mesma borda dessa classe e qual a posicao desta na fila.
// Recalcula as duas pontas de todos os relacionamentos de uma vez, uma vez por frame.
// Feito ponta a ponta seria O(r^2 * c), porque cada uma precisaria varrer todas as
// outras para saber quantas dividem a mesma borda; agrupando por (classe, lado) cai
// para O(r * c).
static void RebuildGeometry(void)
{
    int classCount = GetClassCount();
    if (relationCount == 0 || classCount == 0) return;

    if (geometryCapacity < relationCount)
    {
        geometry = (RelationGeometry *)realloc(geometry, relationCount * sizeof(RelationGeometry));
        endpointClass = (int *)realloc(endpointClass, relationCount * 2 * sizeof(int));
        endpointSide = (int *)realloc(endpointSide, relationCount * 2 * sizeof(int));
        geometryCapacity = relationCount;
    }

    if (sideCapacity < classCount * 4)
    {
        sideTotals = (int *)realloc(sideTotals, classCount * 4 * sizeof(int));
        sideUsed = (int *)realloc(sideUsed, classCount * 4 * sizeof(int));
        sideCapacity = classCount * 4;
    }

    for (int i = 0; i < classCount * 4; i++)
    {
        sideTotals[i] = 0;
        sideUsed[i] = 0;
    }

    for (int i = 0; i < relationCount; i++)
    {
        int fromIndex = FindClassIndexById(relations[i].fromId);
        int toIndex = FindClassIndexById(relations[i].toId);

        geometry[i].valid = (fromIndex != -1 && toIndex != -1);
        if (!geometry[i].valid) continue;

        Rectangle fromBox = GetClassBounds(fromIndex);
        Rectangle toBox = GetClassBounds(toIndex);

        endpointClass[i * 2] = fromIndex;
        endpointSide[i * 2] = GetExitSide(fromBox, GetRectCenter(toBox));
        endpointClass[i * 2 + 1] = toIndex;
        endpointSide[i * 2 + 1] = GetExitSide(toBox, GetRectCenter(fromBox));

        sideTotals[fromIndex * 4 + endpointSide[i * 2]]++;
        sideTotals[toIndex * 4 + endpointSide[i * 2 + 1]]++;
    }

    for (int i = 0; i < relationCount; i++)
    {
        if (!geometry[i].valid) continue;

        for (int end = 0; end < 2; end++)
        {
            int key = endpointClass[i * 2 + end] * 4 + endpointSide[i * 2 + end];
            Vector2 point = GetAnchorOnSide(GetClassBounds(endpointClass[i * 2 + end]),
                                            (BoxSide)endpointSide[i * 2 + end],
                                            sideUsed[key]++, sideTotals[key]);

            if (end == 0) geometry[i].start = point;
            else geometry[i].end = point;
        }
    }
}

static Vector2 GetBorderPoint(Rectangle rect, Vector2 target)
{
    Vector2 center = GetRectCenter(rect);
    float dx = target.x - center.x;
    float dy = target.y - center.y;

    if (dx == 0.0f && dy == 0.0f) return center;

    float halfWidth = rect.width / 2.0f;
    float halfHeight = rect.height / 2.0f;
    float scale;

    if (dx == 0.0f) scale = halfHeight / fabsf(dy);
    else if (dy == 0.0f) scale = halfWidth / fabsf(dx);
    else
    {
        float scaleX = halfWidth / fabsf(dx);
        float scaleY = halfHeight / fabsf(dy);
        scale = (scaleX < scaleY) ? scaleX : scaleY;
    }

    return (Vector2){center.x + dx * scale, center.y + dy * scale};
}

static float DistanceToSegment(Vector2 point, Vector2 start, Vector2 end)
{
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float lengthSquared = dx * dx + dy * dy;

    if (lengthSquared < 0.0001f) return sqrtf((point.x - start.x) * (point.x - start.x) + (point.y - start.y) * (point.y - start.y));

    float t = ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float projX = start.x + t * dx;
    float projY = start.y + t * dy;

    return sqrtf((point.x - projX) * (point.x - projX) + (point.y - projY) * (point.y - projY));
}

static void DrawDashedLine(Vector2 start, Vector2 end, Color color)
{
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 1.0f) return;

    float ux = dx / length;
    float uy = dy / length;

    for (float traveled = 0.0f; traveled < length; traveled += 14.0f)
    {
        float segmentEnd = traveled + 8.0f;
        if (segmentEnd > length) segmentEnd = length;

        DrawLineEx((Vector2){start.x + ux * traveled, start.y + uy * traveled},
                   (Vector2){start.x + ux * segmentEnd, start.y + uy * segmentEnd}, LINE_THICKNESS, color);
    }
}

static void FillTriangle(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    DrawTriangle(a, b, c, color);
    DrawTriangle(a, c, b, color);
}

static void DrawTriangleOrnament(Vector2 tip, Vector2 towards, bool filled, Color color)
{
    float dx = tip.x - towards.x;
    float dy = tip.y - towards.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 0.001f) return;

    float ux = dx / length;
    float uy = dy / length;
    Vector2 base = {tip.x - ux * ORNAMENT_LENGTH, tip.y - uy * ORNAMENT_LENGTH};
    Vector2 left = {base.x - uy * ORNAMENT_HALFWIDTH, base.y + ux * ORNAMENT_HALFWIDTH};
    Vector2 right = {base.x + uy * ORNAMENT_HALFWIDTH, base.y - ux * ORNAMENT_HALFWIDTH};

    FillTriangle(tip, left, right, filled ? color : ThemeCanvas());
    DrawLineEx(tip, left, LINE_THICKNESS, color);
    DrawLineEx(tip, right, LINE_THICKNESS, color);
    DrawLineEx(left, right, LINE_THICKNESS, color);
}

static void DrawDiamondOrnament(Vector2 tip, Vector2 towards, bool filled, Color color)
{
    float dx = towards.x - tip.x;
    float dy = towards.y - tip.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 0.001f) return;

    float ux = dx / length;
    float uy = dy / length;
    Vector2 middle = {tip.x + ux * ORNAMENT_LENGTH, tip.y + uy * ORNAMENT_LENGTH};
    Vector2 back = {tip.x + ux * ORNAMENT_LENGTH * 2.0f, tip.y + uy * ORNAMENT_LENGTH * 2.0f};
    Vector2 left = {middle.x - uy * ORNAMENT_HALFWIDTH, middle.y + ux * ORNAMENT_HALFWIDTH};
    Vector2 right = {middle.x + uy * ORNAMENT_HALFWIDTH, middle.y - ux * ORNAMENT_HALFWIDTH};

    Color fill = filled ? color : ThemeCanvas();
    FillTriangle(tip, left, back, fill);
    FillTriangle(tip, back, right, fill);

    DrawLineEx(tip, left, LINE_THICKNESS, color);
    DrawLineEx(left, back, LINE_THICKNESS, color);
    DrawLineEx(back, right, LINE_THICKNESS, color);
    DrawLineEx(right, tip, LINE_THICKNESS, color);
}

static void DrawOpenArrow(Vector2 tip, Vector2 towards, Color color)
{
    float dx = tip.x - towards.x;
    float dy = tip.y - towards.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 0.001f) return;

    float ux = dx / length;
    float uy = dy / length;
    Vector2 base = {tip.x - ux * ORNAMENT_LENGTH, tip.y - uy * ORNAMENT_LENGTH};

    DrawLineEx(tip, (Vector2){base.x - uy * ORNAMENT_HALFWIDTH, base.y + ux * ORNAMENT_HALFWIDTH}, LINE_THICKNESS, color);
    DrawLineEx(tip, (Vector2){base.x + uy * ORNAMENT_HALFWIDTH, base.y - ux * ORNAMENT_HALFWIDTH}, LINE_THICKNESS, color);
}

static void DrawMultiplicity(const char *text, Vector2 endpoint, Vector2 towards, float offset)
{
    if (text[0] == '\0') return;

    float dx = towards.x - endpoint.x;
    float dy = towards.y - endpoint.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 0.001f) return;

    float ux = dx / length;
    float uy = dy / length;
    float px = -uy;
    float py = ux;

    // Mantem o rotulo sempre do lado de cima da linha (e a direita, quando ela e vertical)
    if (py > 0.0f || (fabsf(py) < 0.01f && px < 0.0f))
    {
        px = -px;
        py = -py;
    }

    Vector2 anchor = {endpoint.x + ux * offset + px * LABEL_DISTANCE,
                      endpoint.y + uy * offset + py * LABEL_DISTANCE};

    int textWidth = MeasureUiText(text, MULTIPLICITY_FONT);
    float textX = anchor.x - textWidth / 2.0f;
    float textY = anchor.y - MULTIPLICITY_FONT / 2.0f;

    DrawRectangle(textX - 3, textY - 2, textWidth + 6, MULTIPLICITY_FONT + 4, ThemeCanvas());
    DrawUiText(text, textX, textY, MULTIPLICITY_FONT, ThemeTextMuted());
}

static bool GetRelationEndpoints(int index, Vector2 *start, Vector2 *end)
{
    if (index >= geometryCapacity || !geometry[index].valid) return false;

    *start = geometry[index].start;
    *end = geometry[index].end;

    return true;
}

static void DrawRelation(int index, Color color)
{
    Vector2 start, end;
    if (!GetRelationEndpoints(index, &start, &end)) return;

    RelationType type = relations[index].type;
    bool dashed = (type == RELATION_DEPENDENCY || type == RELATION_REALIZATION);

    if (dashed) DrawDashedLine(start, end, color);
    else DrawLineEx(start, end, LINE_THICKNESS, color);

    switch (type)
    {
        case RELATION_INHERITANCE:
        case RELATION_REALIZATION:
            DrawTriangleOrnament(end, start, false, color);
            break;
        case RELATION_AGGREGATION:
            DrawDiamondOrnament(start, end, false, color);
            break;
        case RELATION_COMPOSITION:
            DrawDiamondOrnament(start, end, true, color);
            break;
        case RELATION_DEPENDENCY:
            DrawOpenArrow(end, start, color);
            break;
        default:
            break;
    }

    bool diamondAtStart = (type == RELATION_AGGREGATION || type == RELATION_COMPOSITION);
    bool headAtEnd = (type == RELATION_INHERITANCE || type == RELATION_REALIZATION || type == RELATION_DEPENDENCY);

    DrawMultiplicity(relations[index].fromMultiplicity, start, end, diamondAtStart ? ORNAMENT_LENGTH * 2.0f + 10.0f : 16.0f);
    DrawMultiplicity(relations[index].toMultiplicity, end, start, headAtEnd ? ORNAMENT_LENGTH + 10.0f : 16.0f);
}

static int GetRelationAt(Vector2 worldPos)
{
    for (int i = relationCount - 1; i >= 0; i--)
    {
        Vector2 start, end;
        if (!GetRelationEndpoints(i, &start, &end)) continue;

        if (DistanceToSegment(worldPos, start, end) <= LINE_HIT_DISTANCE) return i;
    }

    return -1;
}

static void HandleArmedMode(Vector2 mousePos, int *cursor)
{
    *cursor = MOUSE_CURSOR_CROSSHAIR;

    if (pendingFromId != -1)
    {
        int fromIndex = FindClassIndexById(pendingFromId);
        if (fromIndex != -1)
        {
            Vector2 start = GetBorderPoint(GetClassBounds(fromIndex), mousePos);
            DrawLineEx(start, mousePos, LINE_THICKNESS, ThemeAccent());
            DrawRectangleLinesEx(GetClassBounds(fromIndex), 2, ThemeAccent());
        }
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    int clickedIndex = GetClassIndexAt(mousePos);
    if (clickedIndex == -1)
    {
        CancelRelationMode();
        return;
    }

    int clickedId = GetClassIdByIndex(clickedIndex);

    if (pendingFromId == -1)
    {
        pendingFromId = clickedId;
        return;
    }

    if (clickedId == pendingFromId) return;

    CreateRelation(pendingFromId, clickedId);
    CancelRelationMode();
    consumedClick = true;
}

static Rectangle GetConnectHandle(Rectangle box, BoxSide side)
{
    float half = CONNECT_HANDLE_SIZE / 2.0f;
    Vector2 center = GetRectCenter(box);
    Vector2 point;

    switch (side)
    {
        case SIDE_TOP:    point = (Vector2){center.x, box.y - CONNECT_HANDLE_OFFSET}; break;
        case SIDE_BOTTOM: point = (Vector2){center.x, box.y + box.height + CONNECT_HANDLE_OFFSET}; break;
        case SIDE_LEFT:   point = (Vector2){box.x - CONNECT_HANDLE_OFFSET, center.y}; break;
        default:          point = (Vector2){box.x + box.width + CONNECT_HANDLE_OFFSET, center.y}; break;
    }

    return (Rectangle){point.x - half, point.y - half, CONNECT_HANDLE_SIZE, CONNECT_HANDLE_SIZE};
}

static void DrawConnectHandle(Rectangle handle, BoxSide side, bool hover)
{
    Vector2 center = GetRectCenter(handle);
    float half = CONNECT_HANDLE_SIZE / 2.0f;
    Vector2 tip, left, right;

    switch (side)
    {
        case SIDE_TOP:
            tip = (Vector2){center.x, center.y - half};
            left = (Vector2){center.x - half, center.y + half};
            right = (Vector2){center.x + half, center.y + half};
            break;
        case SIDE_BOTTOM:
            tip = (Vector2){center.x, center.y + half};
            left = (Vector2){center.x + half, center.y - half};
            right = (Vector2){center.x - half, center.y - half};
            break;
        case SIDE_LEFT:
            tip = (Vector2){center.x - half, center.y};
            left = (Vector2){center.x + half, center.y + half};
            right = (Vector2){center.x + half, center.y - half};
            break;
        default:
            tip = (Vector2){center.x + half, center.y};
            left = (Vector2){center.x - half, center.y - half};
            right = (Vector2){center.x - half, center.y + half};
            break;
    }

    FillTriangle(tip, left, right, hover ? ThemeAccent() : Fade(ThemeAccent(), 0.65f));
}

// Classe cuja area (incluindo a margem das setas) contem o mouse.
static int GetClassNearHandles(Vector2 mousePos)
{
    float margin = CONNECT_HANDLE_OFFSET + CONNECT_HANDLE_SIZE;

    for (int i = GetClassCount() - 1; i >= 0; i--)
    {
        Rectangle box = GetClassBounds(i);
        Rectangle expanded = {box.x - margin, box.y - margin, box.width + margin * 2, box.height + margin * 2};

        if (CheckCollisionPointRec(mousePos, expanded)) return i;
    }

    return -1;
}

static void HandleConnectHandles(Vector2 mousePos, int *cursor)
{
    if (connectFromId != -1)
    {
        int fromIndex = FindClassIndexById(connectFromId);
        if (fromIndex == -1)
        {
            connectFromId = -1;
            return;
        }

        *cursor = MOUSE_CURSOR_CROSSHAIR;

        int targetIndex = GetClassIndexAt(mousePos);
        bool validTarget = (targetIndex != -1 && GetClassIdByIndex(targetIndex) != connectFromId);

        DrawLineEx(GetBorderPoint(GetClassBounds(fromIndex), mousePos), mousePos, LINE_THICKNESS, ThemeAccent());
        if (validTarget) DrawRectangleLinesEx(GetClassBounds(targetIndex), 2, ThemeAccent());

        float dx = mousePos.x - connectPressPos.x;
        float dy = mousePos.y - connectPressPos.y;
        if (dx * dx + dy * dy > 25.0f) connectMoved = true;

        // Arrastar da seta e soltar sobre a classe alvo, ou clicar na seta e depois clicar no alvo
        bool finished = (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && connectMoved)
                     || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        if (finished)
        {
            if (validTarget) CreateRelation(connectFromId, GetClassIdByIndex(targetIndex));

            connectFromId = -1;
            consumedClick = true;
        }

        return;
    }

    // Segurando o botao o usuario esta arrastando ou redimensionando uma classe,
    // mas o frame do proprio clique precisa passar para a seta poder ser acionada.
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    int nearIndex = GetClassNearHandles(mousePos);
    if (nearIndex == -1) return;

    Rectangle box = GetClassBounds(nearIndex);

    for (int side = 0; side < 4; side++)
    {
        Rectangle handle = GetConnectHandle(box, (BoxSide)side);
        bool hover = CheckCollisionPointRec(mousePos, handle);

        DrawConnectHandle(handle, (BoxSide)side, hover);

        if (!hover) continue;

        *cursor = MOUSE_CURSOR_POINTING_HAND;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            connectFromId = GetClassIdByIndex(nearIndex);
            connectPressPos = mousePos;
            connectMoved = false;
            consumedClick = true;
            return;
        }
    }
}

void UpdateAndDrawRelations(Camera2D camera, int *cursor)
{
    consumedClick = false;
    PruneDanglingRelations();
    RebuildGeometry();

    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    int hovered = -1;

    if (!relationArmed && !IsMouseOverUi() && GetClassIndexAt(mousePos) == -1)
    {
        hovered = GetRelationAt(mousePos);

        if (hovered != -1)
        {
            *cursor = MOUSE_CURSOR_POINTING_HAND;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                ClearClassSelection();
                selectedRelation = hovered;
                panelFocus = FOCUS_NONE;
                panelError = NULL;
                consumedClick = true;
            }
        }
    }

    for (int i = 0; i < relationCount; i++)
    {
        Color color = ThemeBorder();
        if (i == selectedRelation) color = ThemeAccent();
        else if (i == hovered) color = Fade(ThemeAccent(), 0.6f);

        DrawRelation(i, color);
    }

    if (selectedRelation != -1 && panelFocus == FOCUS_NONE
        && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE)))
    {
        PushHistory();
        RemoveRelation(selectedRelation);
        ClearRelationSelection();
        return;
    }

    if (relationArmed && !IsMouseOverUi()) HandleArmedMode(mousePos, cursor);
    else if (!relationArmed && !IsMouseOverUi()) HandleConnectHandles(mousePos, cursor);
}

static void DrawQuickRow(Rectangle area, float y, char *target, int *cursor)
{
    float quickWidth = (area.width - PANEL_GAP * 2) / 3.0f;

    for (int i = 0; i < QUICK_COUNT; i++)
    {
        Rectangle rect = {area.x + (i % 3) * (quickWidth + PANEL_GAP),
                          y + (i / 3) * (PANEL_ROW_HEIGHT + PANEL_GAP),
                          quickWidth, PANEL_ROW_HEIGHT};

        bool isClear = (i == QUICK_COUNT - 1);
        const char *value = isClear ? "" : quickMultiplicity[i];

        if (PanelButton(rect, quickMultiplicity[i], strcmp(target, value) == 0, ThemeAccent(), 11, cursor))
        {
            PushHistory();
            snprintf(target, MULTIPLICITY_LEN, "%s", value);
        }
    }
}

void DrawRelationProperties(Rectangle area, int *cursor)
{
    if (selectedRelation == -1) return;

    UMLRelation *relation = &relations[selectedRelation];
    float y = area.y;

    if (panelFocus == FOCUS_FROM) AppendTypedChars(relation->fromMultiplicity, MULTIPLICITY_LEN);
    else if (panelFocus == FOCUS_TO) AppendTypedChars(relation->toMultiplicity, MULTIPLICITY_LEN);

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) panelFocus = FOCUS_NONE;

    DrawUiText("Tipo", area.x, y, PANEL_LABEL_FONT, ThemeTextMuted());
    y += 18;

    float typeWidth = (area.width - PANEL_GAP) / 2.0f;
    for (int i = 0; i < RELATION_TYPE_COUNT; i++)
    {
        Rectangle rect = {area.x + (i % 2) * (typeWidth + PANEL_GAP),
                          y + (i / 2) * (PANEL_ROW_HEIGHT + PANEL_GAP),
                          typeWidth, PANEL_ROW_HEIGHT};

        if (PanelButton(rect, relationLabels[i], relation->type == (RelationType)i, ThemeAccent(), 11, cursor))
        {
            panelError = ValidateRelation(selectedRelation, relation->fromId, relation->toId, (RelationType)i);

            if (panelError == NULL)
            {
                PushHistory();
                relation->type = (RelationType)i;
            }
        }
    }
    y += 3 * PANEL_ROW_HEIGHT + 2 * PANEL_GAP + PANEL_GAP;

    if (panelError != NULL)
    {
        DrawUiText(panelError, area.x, y, 12, ThemeDanger());
    }
    y += 20;

    DrawUiText("Multiplicidade na origem", area.x, y, PANEL_LABEL_FONT, ThemeTextMuted());
    y += 18;
    if (PanelField((Rectangle){area.x, y, area.width, PANEL_FIELD_HEIGHT}, relation->fromMultiplicity, panelFocus == FOCUS_FROM, cursor))
    {
        if (panelFocus != FOCUS_FROM) PushHistory();
        panelFocus = FOCUS_FROM;
    }
    y += PANEL_FIELD_HEIGHT + PANEL_GAP;

    DrawQuickRow(area, y, relation->fromMultiplicity, cursor);
    y += 2 * PANEL_ROW_HEIGHT + PANEL_GAP + 16;

    DrawUiText("Multiplicidade no destino", area.x, y, PANEL_LABEL_FONT, ThemeTextMuted());
    y += 18;
    if (PanelField((Rectangle){area.x, y, area.width, PANEL_FIELD_HEIGHT}, relation->toMultiplicity, panelFocus == FOCUS_TO, cursor))
    {
        if (panelFocus != FOCUS_TO) PushHistory();
        panelFocus = FOCUS_TO;
    }
    y += PANEL_FIELD_HEIGHT + PANEL_GAP;

    DrawQuickRow(area, y, relation->toMultiplicity, cursor);

    if (PanelButton((Rectangle){area.x, area.y + area.height - 30, area.width, 28}, "Excluir relacionamento", false, ThemeDanger(), 13, cursor))
    {
        PushHistory();
        RemoveRelation(selectedRelation);
        ClearRelationSelection();
    }
}
