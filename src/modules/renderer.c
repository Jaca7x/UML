#include "../include/renderer.h"

void DrawWorldGrid(int size, float spacing, Color color) {
    for (float i = -size; i <= size; i += spacing) {
        DrawLine(i, -size, i, size, color);
        DrawLine(-size, i, size, i, color);
    }
}