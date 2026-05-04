#include "../include/editor.h"
#include <stdlib.h>
#include <stdio.h>

static int clickCount = 0;
static UMLClass *arrayClass = NULL;


void AddUMLClass(Camera2D camera)
{    
    Vector2 worldPos = GetScreenToWorld2D(camera.target, camera);

    clickCount++;
    arrayClass = (UMLClass *)realloc(arrayClass, clickCount * sizeof(UMLClass));
    arrayClass[clickCount - 1].bounds.x / 2.0f - worldPos.x;
    arrayClass[clickCount - 1].bounds.y / 2.0f - worldPos.y;
        
    arrayClass[clickCount - 1].bounds.width = 150;
    arrayClass[clickCount - 1].bounds.height = 100;

    arrayClass[clickCount - 1].isDragging = false;
    arrayClass[clickCount - 1].isSelected = false;
}

void UpdateAndDrawBoxes(Camera2D camera, int *cursor) {
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    
    for (int i = clickCount - 1; i >= 0; i--)
    {
        if (CheckCollisionPointRec(mousePos, arrayClass[i].bounds))
        {
            *cursor = MOUSE_CURSOR_POINTING_HAND;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                arrayClass[i].isDragging = true;  
                arrayClass[i].dragOffSet.x = mousePos.x - arrayClass[i].bounds.x;
                arrayClass[i].dragOffSet.y = mousePos.y - arrayClass[i].bounds.y;
                break; 
            } 
        }
    } 
    
    for (int i = 0; i < clickCount; i++) {
        Color colorClass = BLACK;

        if (arrayClass[i].isDragging)
        {   
            *cursor = MOUSE_CURSOR_RESIZE_ALL;
            colorClass = BLUE;

            arrayClass[i].bounds.x = mousePos.x - arrayClass[i].dragOffSet.x;
            arrayClass[i].bounds.y = mousePos.y - arrayClass[i].dragOffSet.y;  
        }

        arrayClass[i].id = i;
        
        DrawRectangleLinesEx(arrayClass[i].bounds, 2, colorClass);
        DrawText("Nova Classe", arrayClass[i].bounds.x + 30, arrayClass[i].bounds.y + 10, 16, RED); 
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        for (int i = 0; i < clickCount; i++) {
            arrayClass[i].isDragging = false;
        }
    }  
}