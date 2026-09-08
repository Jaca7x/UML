#ifndef RENDERER_H
#define RENDERER_H
#include "../../lib/raylib.h"

// Espacamento da grade do mundo. O snap usa o mesmo valor do desenho.
#define WORLD_GRID_SPACING 50.0f

void DrawWorldGrid(int size, float spacing, Color color);

#endif