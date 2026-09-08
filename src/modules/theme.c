#include "../include/theme.h"

typedef struct
{
    Color canvas;
    Color grid;
    Color menuBar;
    Color panel;
    Color surface;
    Color border;
    Color text;
    Color textMuted;
    Color accent;
    Color danger;
    Color success;
}Palette;

static const Palette lightPalette = {
    .canvas    = {245, 245, 245, 255},
    .grid      = {205, 205, 205, 255},
    .menuBar   = {200, 200, 200, 255},
    .panel     = {236, 236, 236, 255},
    .surface   = {253, 253, 253, 255},
    .border    = {130, 130, 130, 255},
    .text      = { 20,  20,  20, 255},
    .textMuted = { 95,  95,  95, 255},
    .accent    = {  0,  90, 220, 255},
    .danger    = {200,  35,  45, 255},
    .success   = {  0, 120,  60, 255}
};

// Nao e o claro invertido: no escuro o contorno precisa clarear em vez de
// escurecer, e o azul/vermelho puros vibram demais sobre fundo escuro.
static const Palette darkPalette = {
    .canvas    = { 28,  30,  36, 255},
    .grid      = { 46,  49,  58, 255},
    .menuBar   = { 38,  41,  49, 255},
    .panel     = { 34,  37,  44, 255},
    .surface   = { 50,  54,  64, 255},
    .border    = {100, 106, 120, 255},
    .text      = {232, 234, 240, 255},
    .textMuted = {150, 156, 170, 255},
    .accent    = { 90, 160, 255, 255},
    .danger    = {255, 115, 115, 255},
    .success   = {110, 210, 150, 255}
};

static bool darkMode = false;

void ToggleTheme(void)
{
    darkMode = !darkMode;
}

bool IsDarkMode(void)
{
    return darkMode;
}

static const Palette *Current(void)
{
    return darkMode ? &darkPalette : &lightPalette;
}

Color ThemeCanvas(void)    { return Current()->canvas; }
Color ThemeGrid(void)      { return Current()->grid; }
Color ThemeMenuBar(void)   { return Current()->menuBar; }
Color ThemePanel(void)     { return Current()->panel; }
Color ThemeSurface(void)   { return Current()->surface; }
Color ThemeBorder(void)    { return Current()->border; }
Color ThemeText(void)      { return Current()->text; }
Color ThemeTextMuted(void) { return Current()->textMuted; }
Color ThemeAccent(void)    { return Current()->accent; }
Color ThemeDanger(void)    { return Current()->danger; }
Color ThemeSuccess(void)   { return Current()->success; }
