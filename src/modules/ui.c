#include "../include/ui.h"
#include "editor.h"
#include "relations.h"
#include "uifont.h"
#include "theme.h"
#include "storage.h"
#include <stdio.h>

#define MENU_BAR_HEIGHT 50
#define BUTTON_PADDING   10
#define BUTTON_WIDTH    105
#define BUTTON_HEIGHT    30
#define BUTTON_GAP        8
#define MENU_BUTTON_COUNT 7

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
                        (MENU_BAR_HEIGHT - BUTTON_HEIGHT) / 2.0f, BUTTON_WIDTH, BUTTON_HEIGHT};
}

bool IsMouseOverUi(void)
{
    Vector2 mouse = GetMousePosition();

    return CheckCollisionPointRec(mouse, GetMenuBarBounds()) || CheckCollisionPointRec(mouse, GetPanelBounds());
}

static void DrawMenuButton(Rectangle button, const char *label, bool isArmed, int *cursor, void (*onClick)(void))
{
    Color borderColor = ThemeBorder();
    Color textColor = ThemeText();

    if (CheckCollisionPointRec(GetMousePosition(), button))
    {
        borderColor = ThemeAccent();
        textColor = ThemeAccent();
        *cursor = MOUSE_CURSOR_POINTING_HAND;

        if (!IsClassNameRequired() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) onClick();
    }

    if (isArmed) { borderColor = ThemeAccent(); textColor = ThemeAccent(); }

    DrawRectangleRec(button, isArmed ? Fade(ThemeAccent(), 0.20f) : ThemeSurface());
    DrawUiText(label, button.x + 10, button.y + 8, 14, textColor);
    DrawRectangleLinesEx(button, 1, borderColor);
}

void DrawUi(int *cursor) {

    Rectangle menuBar = GetMenuBarBounds();
    Rectangle panel = GetPanelBounds();

    DrawRectangleRec(menuBar, ThemeMenuBar());
    DrawLine(0, menuBar.height, GetScreenWidth(), menuBar.height, ThemeBorder());

    DrawMenuButton(GetButtonBounds(0), "Criar Classe", IsPlacingClass(), cursor, ArmClassPlacement);
    DrawMenuButton(GetButtonBounds(1), "Relacionar", IsRelationModeArmed(), cursor, ArmRelationMode);
    DrawMenuButton(GetButtonBounds(2), "Salvar", false, cursor, SaveToDefaultFile);
    DrawMenuButton(GetButtonBounds(3), "Carregar", false, cursor, LoadFromDefaultFile);
    DrawMenuButton(GetButtonBounds(4), "Tela Cheia", IsWindowFullscreen(), cursor, ToggleFullscreenMode);
    DrawMenuButton(GetButtonBounds(5), IsDarkMode() ? "Tema Claro" : "Tema Escuro", IsDarkMode(), cursor, ToggleTheme);
    DrawMenuButton(GetButtonBounds(6), "Alinhar", IsSnapToGridEnabled(), cursor, ToggleSnapToGrid);

    bool overButton = false;
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        if (CheckCollisionPointRec(GetMousePosition(), GetButtonBounds(i))) overButton = true;
    }

    if (!overButton && CheckCollisionPointRec(GetMousePosition(), menuBar)) *cursor = MOUSE_CURSOR_DEFAULT;

    if (GetTime() < statusUntil)
    {
        float statusX = GetButtonBounds(MENU_BUTTON_COUNT - 1).x + BUTTON_WIDTH + 20;
        DrawUiText(statusMessage, statusX, (MENU_BAR_HEIGHT - 14) / 2.0f, 14, statusColor);
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
