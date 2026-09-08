#include "../include/ui.h"
#include "editor.h"
#include "relations.h"
#include "uifont.h"

#define MENU_BAR_HEIGHT 50
#define BUTTON_PADDING   10
#define BUTTON_WIDTH    120
#define BUTTON_HEIGHT    30
#define BUTTON_GAP        8

#define PANEL_WIDTH     300
#define PANEL_PADDING    14

#define PANEL_BACKGROUND (Color){236, 236, 236, 255}

static int windowedWidth = 0;
static int windowedHeight = 0;

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
    Color borderColor = BLACK;
    Color textColor = BLACK;

    if (CheckCollisionPointRec(GetMousePosition(), button))
    {
        borderColor = BLUE;
        textColor = BLUE;
        *cursor = MOUSE_CURSOR_POINTING_HAND;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) onClick();
    }

    if (isArmed) { borderColor = BLUE; textColor = BLUE; }

    DrawRectangleRec(button, isArmed ? Fade(BLUE, 0.15f) : RAYWHITE);
    DrawUiText(label, button.x + 10, button.y + 8, 14, textColor);
    DrawRectangleLinesEx(button, 1, borderColor);
}

void DrawUi(int *cursor) {

    Rectangle menuBar = GetMenuBarBounds();
    Rectangle panel = GetPanelBounds();

    DrawRectangleRec(menuBar, LIGHTGRAY);
    DrawLine(0, menuBar.height, GetScreenWidth(), menuBar.height, GRAY);

    DrawMenuButton(GetButtonBounds(0), "Criar Classe", IsPlacingClass(), cursor, ArmClassPlacement);
    DrawMenuButton(GetButtonBounds(1), "Relacionar", IsRelationModeArmed(), cursor, ArmRelationMode);
    DrawMenuButton(GetButtonBounds(2), "Tela Cheia", IsWindowFullscreen(), cursor, ToggleFullscreenMode);

    bool overButton = false;
    for (int i = 0; i < 3; i++)
    {
        if (CheckCollisionPointRec(GetMousePosition(), GetButtonBounds(i))) overButton = true;
    }

    if (!overButton && CheckCollisionPointRec(GetMousePosition(), menuBar)) *cursor = MOUSE_CURSOR_DEFAULT;

    DrawRectangleRec(panel, PANEL_BACKGROUND);
    DrawLine(panel.x, panel.y, panel.x, panel.y + panel.height, GRAY);

    Rectangle content = {panel.x + PANEL_PADDING, panel.y + PANEL_PADDING,
                          panel.width - PANEL_PADDING * 2, panel.height - PANEL_PADDING * 2};

    DrawUiText("PROPRIEDADES", content.x, content.y, 14, BLACK);
    DrawLine(content.x, content.y + 22, content.x + content.width, content.y + 22, GRAY);

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
                 content.x, content.y + 8, 14, BLACK);
    }
}
