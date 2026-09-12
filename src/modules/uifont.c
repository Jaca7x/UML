#include "../include/uifont.h"
#include <stddef.h>
#include <stdlib.h>

// Ordem de busca: fonte do projeto (versionavel), depois a do sistema.
// Sem nenhuma delas, cai na fonte bitmap padrao da raylib.
static const char *fontCandidates[] = {
    "assets/fonts/ui.ttf",
    "C:/Windows/Fonts/segoeui.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/System/Library/Fonts/Helvetica.ttc"
};

// Um atlas por tamanho usado na interface. Desenhar em tamanho diferente do
// atlas obriga a escalar a textura, que e justamente o que borra o texto.
#define UI_FONT_VARIANTS 3

static const int fontSizes[UI_FONT_VARIANTS] = {12, 14, 16};
static Font fonts[UI_FONT_VARIANTS] = {0};
static bool customFontLoaded = false;

// ASCII imprimivel mais as aspas angulares do estereotipo UML («interface»),
// que ficam fora da faixa ASCII e sairiam como caixinha sem isto.
#define ASCII_COUNT      95
#define UI_CODEPOINTS (ASCII_COUNT + 2)

static int codepoints[UI_CODEPOINTS];

static void BuildCodepoints(void)
{
    for (int i = 0; i < ASCII_COUNT; i++) codepoints[i] = 32 + i;

    codepoints[ASCII_COUNT] = 0x00AB;     // <<
    codepoints[ASCII_COUNT + 1] = 0x00BB; // >>
}

static const char *FindFontPath(void)
{
    int candidateCount = sizeof(fontCandidates) / sizeof(fontCandidates[0]);

    for (int i = 0; i < candidateCount; i++)
    {
        if (FileExists(fontCandidates[i])) return fontCandidates[i];
    }

    return NULL;
}

void LoadUiFont(void)
{
    const char *path = FindFontPath();
    BuildCodepoints();

    if (path != NULL)
    {
        for (int i = 0; i < UI_FONT_VARIANTS; i++)
        {
            fonts[i] = LoadFontEx(path, fontSizes[i], codepoints, UI_CODEPOINTS);

            if (fonts[i].texture.id == 0)
            {
                for (int done = 0; done < i; done++) UnloadFont(fonts[done]);
                customFontLoaded = false;
                break;
            }

            SetTextureFilter(fonts[i].texture, TEXTURE_FILTER_BILINEAR);
            customFontLoaded = true;
        }

        if (customFontLoaded) return;
    }

    for (int i = 0; i < UI_FONT_VARIANTS; i++) fonts[i] = GetFontDefault();
}

void UnloadUiFont(void)
{
    if (!customFontLoaded) return;

    for (int i = 0; i < UI_FONT_VARIANTS; i++) UnloadFont(fonts[i]);
}

// Atlas mais proximo do tamanho pedido: o texto e desenhado no tamanho dele,
// nunca no pedido, para a escala ficar sempre 1:1.
static int PickVariant(int fontSize)
{
    int best = 0;
    int bestDiff = abs(fontSize - fontSizes[0]);

    for (int i = 1; i < UI_FONT_VARIANTS; i++)
    {
        int diff = abs(fontSize - fontSizes[i]);

        if (diff < bestDiff)
        {
            bestDiff = diff;
            best = i;
        }
    }

    return best;
}

void DrawUiText(const char *text, float x, float y, int fontSize, Color color)
{
    // Posicao inteira: em coordenada quebrada o glifo cai entre dois pixels e borra
    Vector2 position = {(float)(int)(x + 0.5f), (float)(int)(y + 0.5f)};

    if (!customFontLoaded)
    {
        DrawTextEx(GetFontDefault(), text, position, (float)fontSize, fontSize / 10.0f, color);
        return;
    }

    int variant = PickVariant(fontSize);
    DrawTextEx(fonts[variant], text, position, (float)fontSizes[variant], 1.0f, color);
}

int MeasureUiText(const char *text, int fontSize)
{
    if (!customFontLoaded) return (int)MeasureTextEx(GetFontDefault(), text, (float)fontSize, fontSize / 10.0f).x;

    int variant = PickVariant(fontSize);

    return (int)MeasureTextEx(fonts[variant], text, (float)fontSizes[variant], 1.0f).x;
}
