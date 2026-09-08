#include "../include/widgets.h"
#include "../include/uifont.h"
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
