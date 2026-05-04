#include "../include/ui.h"
#include "editor.h"

void DrawUi(Rectangle ui, Camera2D camera, int *cursor) {

    Color hover = BLACK;
    int thickness = 1;
    int fontSize = 14;

    int offsetX = ui.x + 5;
    int offsetY = ui.y + 5;

    if (CheckCollisionPointRec(GetMousePosition(), ui))
    {
        hover = BLUE;
        *cursor = MOUSE_CURSOR_POINTING_HAND; 

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            AddUMLClass(camera);
        }
    }

    DrawText("Criar Classe", offsetX, offsetY, fontSize, hover);
    DrawRectangleLinesEx(ui, thickness, hover);    
}