#ifndef EDITOR_H
#define EDITOR_H
#include "../../lib/raylib.h"

typedef struct 
{

    Rectangle bounds;

    int id;

    char name[64];
    char atributes[256];
    char methods[256];

    bool isSelected;
    bool isDragging;
    Vector2 dragOffSet;
}UMLClass;

void AddUMLClass (Camera2D camera);
void UpdateAndDrawBoxes(Camera2D camera, int *cursor);

#endif