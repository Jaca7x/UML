#include "../include/widgets.h"
#include "../include/uifont.h"
#include "../include/theme.h"
#include "../include/theme.h"
#include <stdio.h>
#include <string.h>

#define FIELD_TEXT_LEN 128
#define LABEL_FONT_SIZE 13
#define FIELD_FONT_SIZE 14

void AppendTypedChars(char *buffer, int capacity)
{
    // Com Ctrl segurado a tecla e atalho (Ctrl+Z), nao texto
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) return;

    int key = GetCharPressed();
    while (key > 0)
    {
        int len = (int)strlen(buffer);
        if (key >= 32 && key < 127 && len < capacity - 1)
        {
            buffer[len] = (char)key;
            buffer[len + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
    {
        int len = (int)strlen(buffer);
        if (len > 0) buffer[len - 1] = '\0';
    }
}

bool PanelButton(Rectangle rect, const char *label, bool selected, Color accent, int fontSize, int *cursor)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    if (hover) *cursor = MOUSE_CURSOR_POINTING_HAND;

    DrawRectangleRec(rect, selected ? Fade(accent, 0.20f) : ThemeSurface());
    DrawRectangleLinesEx(rect, selected ? 2 : 1, selected ? accent : ThemeBorder());

    int textWidth = MeasureUiText(label, fontSize);
    DrawUiText(label, rect.x + (rect.width - textWidth) / 2.0f, rect.y + (rect.height - fontSize) / 2.0f,
               fontSize, selected ? accent : ThemeText());

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool PanelField(Rectangle rect, const char *value, bool focused, int *cursor)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    if (hover) *cursor = MOUSE_CURSOR_IBEAM;

    DrawRectangleRec(rect, ThemeSurface());
    DrawRectangleLinesEx(rect, focused ? 2 : 1, focused ? ThemeAccent() : ThemeBorder());

    bool blinkOn = focused && ((int)(GetTime() * 2.0)) % 2 == 0;
    char display[FIELD_TEXT_LEN];
    snprintf(display, sizeof(display), "%s%s", value, blinkOn ? "|" : "");

    DrawUiText(display, rect.x + 6, rect.y + (rect.height - FIELD_FONT_SIZE) / 2.0f, FIELD_FONT_SIZE, ThemeText());

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void PanelLabel(const char *text, float x, float y)
{
    DrawUiText(text, x, y, LABEL_FONT_SIZE, ThemeTextMuted());
}

int PanelTabs(Rectangle area, const char **names, int count, int active, int *cursor)
{
    float width = area.width / count;

    for (int i = 0; i < count; i++)
    {
        Rectangle tab = {area.x + i * width, area.y, width, area.height};
        bool selected = (i == active);
        bool hover = CheckCollisionPointRec(GetMousePosition(), tab);

        if (hover)
        {
            *cursor = MOUSE_CURSOR_POINTING_HAND;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) active = i;
        }

        Color textColor = selected ? ThemeAccent() : (hover ? ThemeText() : ThemeTextMuted());
        int textWidth = MeasureUiText(names[i], 12);

        DrawUiText(names[i], tab.x + (tab.width - textWidth) / 2.0f, tab.y + 6, 12, textColor);

        // Sublinhado marca a ativa sem depender so da cor
        DrawRectangle(tab.x + 4, tab.y + tab.height - 2, tab.width - 8, 2,
                      selected ? ThemeAccent() : Fade(ThemeBorder(), 0.4f));
    }

    return active;
}
