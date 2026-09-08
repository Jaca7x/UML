#include "../include/ui.h"
#include "editor.h"
#include "relations.h"
#include "uifont.h"
#include "theme.h"
#include "storage.h"
#include <stdio.h>

#define TAB_HEIGHT       26
#define TAB_WIDTH        84
#define MENU_BAR_HEIGHT  (TAB_HEIGHT + 52)
#define BUTTON_PADDING   10
#define BUTTON_WIDTH     64
#define BUTTON_HEIGHT    44
#define BUTTON_GAP        4

#define ICON_SIZE        18
#define ICON_THICKNESS  1.6f
#define CAPTION_FONT     12

#define STATUS_DURATION 2.5
#define STATUS_LEN       96

#define PANEL_WIDTH     300
#define PANEL_PADDING    14

static int windowedWidth = 0;
static int windowedHeight = 0;

static char statusMessage[STATUS_LEN] = {0};
static Color statusColor = BLACK;
static double statusUntil = 0.0;

static void SetStatus(const char *message, Color color)
{
    snprintf(statusMessage, sizeof(statusMessage), "%s", message);
    statusColor = color;
    statusUntil = GetTime() + STATUS_DURATION;
}

void ShowUiStatus(const char *message, bool success)
{
    SetStatus(message, success ? ThemeSuccess() : ThemeDanger());
}

static void SaveToDefaultFile(void)
{
    if (SaveDiagram(DEFAULT_DIAGRAM_PATH)) SetStatus("Diagrama salvo em " DEFAULT_DIAGRAM_PATH, ThemeSuccess());
    else SetStatus("Nao foi possivel salvar o arquivo", ThemeDanger());
}

static void LoadFromDefaultFile(void)
{
    if (LoadDiagram(DEFAULT_DIAGRAM_PATH)) SetStatus("Diagrama carregado", ThemeSuccess());
    else SetStatus("Nao encontrei " DEFAULT_DIAGRAM_PATH, ThemeDanger());
}

void ToggleFullscreenMode(void)
{
    if (IsWindowFullscreen())
    {
        ToggleFullscreen();
        if (windowedWidth > 0) SetWindowSize(windowedWidth, windowedHeight);
        return;
    }

    windowedWidth = GetScreenWidth();
    windowedHeight = GetScreenHeight();

    int display = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
    ToggleFullscreen();
}

static Rectangle GetMenuBarBounds(void)
{
    return (Rectangle){0, 0, (float)GetScreenWidth(), MENU_BAR_HEIGHT};
}

static Rectangle GetPanelBounds(void)
{
    return (Rectangle){(float)(GetScreenWidth() - PANEL_WIDTH), MENU_BAR_HEIGHT,
                        PANEL_WIDTH, (float)GetScreenHeight() - MENU_BAR_HEIGHT};
}

static Rectangle GetButtonBounds(int index)
{
    return (Rectangle){BUTTON_PADDING + index * (BUTTON_WIDTH + BUTTON_GAP),
                        TAB_HEIGHT + 4, BUTTON_WIDTH, BUTTON_HEIGHT};
}

static Rectangle GetTabBounds(int index)
{
    return (Rectangle){BUTTON_PADDING + index * TAB_WIDTH, 0, TAB_WIDTH, TAB_HEIGHT};
}

bool IsMouseOverUi(void)
{
    Vector2 mouse = GetMousePosition();

    return CheckCollisionPointRec(mouse, GetMenuBarBounds()) || CheckCollisionPointRec(mouse, GetPanelBounds());
}


// Os icones sao desenhados com primitivas porque a fonte carregada so tem os
// 95 caracteres ASCII: emoji e simbolos Unicode nao renderizam.

typedef enum
{
    ICON_CLASS = 0,
    ICON_RELATION,
    ICON_SAVE,
    ICON_LOAD,
    ICON_FULLSCREEN,
    ICON_THEME,
    ICON_GRID
}IconKind;

static void IconClass(Rectangle a, Color tint)
{
    DrawRectangleLinesEx(a, ICON_THICKNESS, tint);
    DrawLineEx((Vector2){a.x, a.y + a.height * 0.36f},
               (Vector2){a.x + a.width, a.y + a.height * 0.36f}, ICON_THICKNESS, tint);
}

static void IconRelation(Rectangle a, Color tint)
{
    float box = a.width * 0.36f;

    DrawRectangleLinesEx((Rectangle){a.x, a.y, box, box}, ICON_THICKNESS, tint);
    DrawRectangleLinesEx((Rectangle){a.x + a.width - box, a.y + a.height - box, box, box}, ICON_THICKNESS, tint);
    DrawLineEx((Vector2){a.x + box, a.y + box},
               (Vector2){a.x + a.width - box, a.y + a.height - box}, ICON_THICKNESS, tint);
}

// direction = 1 desce (salvar), -1 sobe (abrir)
static void IconArrowTray(Rectangle a, Color tint, int direction)
{
    float midX = a.x + a.width / 2.0f;
    float head = a.width * 0.26f;
    float top = a.y;
    float arrowEnd = a.y + a.height * 0.60f;
    float tipY = (direction > 0) ? arrowEnd : top;
    float tailY = (direction > 0) ? top : arrowEnd;

    DrawLineEx((Vector2){midX, tailY}, (Vector2){midX, tipY}, ICON_THICKNESS, tint);

    Vector2 tip = {midX, tipY};
    Vector2 left = {midX - head, tipY - direction * head};
    Vector2 right = {midX + head, tipY - direction * head};

    DrawTriangle(tip, left, right, tint);
    DrawTriangle(tip, right, left, tint);

    DrawLineEx((Vector2){a.x, a.y + a.height}, (Vector2){a.x + a.width, a.y + a.height}, ICON_THICKNESS, tint);
}

static void IconFullscreen(Rectangle a, Color tint)
{
    float arm = a.width * 0.34f;

    for (int corner = 0; corner < 4; corner++)
    {
        float x = (corner == 1 || corner == 2) ? a.x + a.width : a.x;
        float y = (corner == 2 || corner == 3) ? a.y + a.height : a.y;
        float dx = (corner == 1 || corner == 2) ? -arm : arm;
        float dy = (corner == 2 || corner == 3) ? -arm : arm;

        DrawLineEx((Vector2){x, y}, (Vector2){x + dx, y}, ICON_THICKNESS, tint);
        DrawLineEx((Vector2){x, y}, (Vector2){x, y + dy}, ICON_THICKNESS, tint);
    }
}

static void IconTheme(Rectangle a, Color tint)
{
    Vector2 center = {a.x + a.width / 2.0f, a.y + a.height / 2.0f};
    float radius = a.width / 2.0f;

    DrawCircleSector(center, radius, 180.0f, 360.0f, 24, tint);
    DrawCircleLines((int)center.x, (int)center.y, radius, tint);
}

static void IconGrid(Rectangle a, Color tint)
{
    for (int i = 1; i < 3; i++)
    {
        float t = i / 3.0f;

        DrawLineEx((Vector2){a.x + a.width * t, a.y},
                   (Vector2){a.x + a.width * t, a.y + a.height}, 1.2f, tint);
        DrawLineEx((Vector2){a.x, a.y + a.height * t},
                   (Vector2){a.x + a.width, a.y + a.height * t}, 1.2f, tint);
    }

    DrawRectangleLinesEx(a, ICON_THICKNESS, tint);
}

static void DrawIcon(IconKind kind, Rectangle area, Color tint)
{
    switch (kind)
    {
        case ICON_CLASS:      IconClass(area, tint); break;
        case ICON_RELATION:   IconRelation(area, tint); break;
        case ICON_SAVE:       IconArrowTray(area, tint, 1); break;
        case ICON_LOAD:       IconArrowTray(area, tint, -1); break;
        case ICON_FULLSCREEN: IconFullscreen(area, tint); break;
        case ICON_THEME:      IconTheme(area, tint); break;
        default:              IconGrid(area, tint); break;
    }
}


typedef struct
{
    IconKind icon;
    const char *caption;
    bool (*isArmed)(void);
    void (*onClick)(void);
}ToolbarButton;

typedef struct
{
    const char *name;
    const ToolbarButton *buttons;
    int count;
}ToolbarTab;

static const ToolbarButton fileButtons[] = {
    {ICON_SAVE, "salvar", NULL, SaveToDefaultFile},
    {ICON_LOAD, "abrir",  NULL, LoadFromDefaultFile}
};

static const ToolbarButton insertButtons[] = {
    {ICON_CLASS,    "classe", IsPlacingClass,      ArmClassPlacement},
    {ICON_RELATION, "ligar",  IsRelationModeArmed, ArmRelationMode}
};

static const ToolbarButton viewButtons[] = {
    {ICON_FULLSCREEN, "tela",    IsWindowFullscreen,  ToggleFullscreenMode},
    {ICON_THEME,      "tema",    IsDarkMode,          ToggleTheme},
    {ICON_GRID,       "alinhar", IsSnapToGridEnabled, ToggleSnapToGrid}
};

static const ToolbarTab tabs[] = {
    {"Arquivo", fileButtons,   2},
    {"Inserir", insertButtons, 2},
    {"Exibir",  viewButtons,   3}
};

#define TAB_COUNT 3

// Inserir e a aba de trabalho: e de onde nascem classes e relacionamentos
static int activeTab = 1;

static void DrawTabStrip(int *cursor)
{
    for (int i = 0; i < TAB_COUNT; i++)
    {
        Rectangle tab = GetTabBounds(i);
        bool active = (i == activeTab);
        bool hover = CheckCollisionPointRec(GetMousePosition(), tab);

        if (hover)
        {
            *cursor = MOUSE_CURSOR_POINTING_HAND;
            if (!IsClassNameRequired() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) activeTab = i;
        }

        if (active) DrawRectangleRec(tab, ThemeSurface());

        Color textColor = active ? ThemeAccent() : (hover ? ThemeText() : ThemeTextMuted());
        int width = MeasureUiText(tabs[i].name, 13);
        DrawUiText(tabs[i].name, tab.x + (tab.width - width) / 2.0f, tab.y + 6, 13, textColor);

        // Sublinhado marca a aba ativa sem depender so da cor do texto
        if (active)
        {
            DrawRectangle(tab.x, tab.y + tab.height - 2, tab.width, 2, ThemeAccent());
        }
    }
}

static void DrawMenuButton(Rectangle button, IconKind icon, const char *caption, bool isArmed,
                           int *cursor, void (*onClick)(void))
{
    Color borderColor = ThemeBorder();
    Color contentColor = ThemeText();

    if (CheckCollisionPointRec(GetMousePosition(), button))
    {
        borderColor = ThemeAccent();
        contentColor = ThemeAccent();
        *cursor = MOUSE_CURSOR_POINTING_HAND;

        if (!IsClassNameRequired() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) onClick();
    }

    if (isArmed) { borderColor = ThemeAccent(); contentColor = ThemeAccent(); }

    DrawRectangleRec(button, isArmed ? Fade(ThemeAccent(), 0.20f) : ThemeSurface());
    DrawRectangleLinesEx(button, 1, borderColor);

    Rectangle iconArea = {button.x + (button.width - ICON_SIZE) / 2.0f, button.y + 7, ICON_SIZE, ICON_SIZE};
    DrawIcon(icon, iconArea, contentColor);

    int captionWidth = MeasureUiText(caption, CAPTION_FONT);
    DrawUiText(caption, button.x + (button.width - captionWidth) / 2.0f,
               button.y + button.height - CAPTION_FONT - 5, CAPTION_FONT, contentColor);
}

void DrawUi(int *cursor) {

    Rectangle menuBar = GetMenuBarBounds();
    Rectangle panel = GetPanelBounds();

    DrawRectangleRec(menuBar, ThemeMenuBar());
    DrawLine(0, menuBar.height, GetScreenWidth(), menuBar.height, ThemeBorder());

    DrawTabStrip(cursor);

    const ToolbarTab *tab = &tabs[activeTab];

    for (int i = 0; i < tab->count; i++)
    {
        bool armed = (tab->buttons[i].isArmed != NULL) && tab->buttons[i].isArmed();
        DrawMenuButton(GetButtonBounds(i), tab->buttons[i].icon, tab->buttons[i].caption,
                       armed, cursor, tab->buttons[i].onClick);
    }

    bool overControl = false;

    for (int i = 0; i < TAB_COUNT && !overControl; i++)
    {
        if (CheckCollisionPointRec(GetMousePosition(), GetTabBounds(i))) overControl = true;
    }

    for (int i = 0; i < tab->count && !overControl; i++)
    {
        if (CheckCollisionPointRec(GetMousePosition(), GetButtonBounds(i))) overControl = true;
    }

    if (!overControl && CheckCollisionPointRec(GetMousePosition(), menuBar)) *cursor = MOUSE_CURSOR_DEFAULT;

    if (GetTime() < statusUntil)
    {
        float statusX = GetButtonBounds(tab->count - 1).x + BUTTON_WIDTH + 20;
        DrawUiText(statusMessage, statusX, TAB_HEIGHT + 18, 14, statusColor);
    }

    // Cobre o canvas, mas nao o painel: o campo do nome fica sendo a unica
    // coisa com que da para interagir
    if (IsClassNameRequired())
    {
        Rectangle canvas = {0, MENU_BAR_HEIGHT, panel.x, (float)GetScreenHeight() - MENU_BAR_HEIGHT};
        DrawRectangleRec(canvas, Fade(BLACK, 0.55f));

        const char *title = "Esta classe precisa de um nome";
        const char *hint = "Preencha o campo Nome da classe no painel ao lado para continuar.";

        int titleWidth = MeasureUiText(title, 16);
        int hintWidth = MeasureUiText(hint, 14);
        int boxWidth = ((titleWidth > hintWidth) ? titleWidth : hintWidth) + 48;

        Rectangle warning = {canvas.x + (canvas.width - boxWidth) / 2.0f,
                             canvas.y + canvas.height / 2.0f - 45, (float)boxWidth, 90};

        DrawRectangleRec(warning, ThemeSurface());
        DrawRectangleLinesEx(warning, 3, ThemeDanger());
        DrawUiText(title, warning.x + (warning.width - titleWidth) / 2.0f, warning.y + 24, 16, ThemeDanger());
        DrawUiText(hint, warning.x + (warning.width - hintWidth) / 2.0f, warning.y + 52, 14, ThemeText());
    }

    DrawRectangleRec(panel, ThemePanel());
    DrawLine(panel.x, panel.y, panel.x, panel.y + panel.height, ThemeBorder());

    Rectangle content = {panel.x + PANEL_PADDING, panel.y + PANEL_PADDING,
                          panel.width - PANEL_PADDING * 2, panel.height - PANEL_PADDING * 2};

    DrawUiText("PROPRIEDADES", content.x, content.y, 14, ThemeText());
    DrawLine(content.x, content.y + 22, content.x + content.width, content.y + 22, ThemeBorder());

    content.y += 34;
    content.height -= 34;

    if (HasSelectedRelation())
    {
        DrawRelationProperties(content, cursor);
    }
    else if (HasSelectedClass())
    {
        DrawClassProperties(content, cursor);
    }
    else
    {
        DrawUiText("Selecione uma classe ou\num relacionamento para\neditar as propriedades.",
                 content.x, content.y + 8, 14, ThemeTextMuted());
    }
}
