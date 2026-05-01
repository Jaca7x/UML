#include "../include/editor.h"
#include <stdlib.h>

void UpdateAndDrawBoxes(Camera2D camera) {
    static int clickCount = 0;
    static Vector2 *arrayPositions = NULL;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        clickCount++;
        arrayPositions = (Vector2 *)realloc(arrayPositions, clickCount * sizeof(Vector2));
        arrayPositions[clickCount - 1] = GetScreenToWorld2D(GetMousePosition(), camera);
    }

    for (int i = 0; i < clickCount; i++) {
        DrawRectangleLines(arrayPositions[i].x, arrayPositions[i].y, 100, 40, BLACK);
    }
}