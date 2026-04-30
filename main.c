#include "src/lib/raylib.h"
#include <stdlib.h>
#include <math.h>

void checkClick(Camera2D camera)
{
    static int clickCount = 0;
    static Vector2 *arrayPositions = NULL;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        clickCount++; 

        arrayPositions = (Vector2 *) realloc(arrayPositions, clickCount * sizeof(Vector2));

        arrayPositions[clickCount - 1] = GetScreenToWorld2D(GetMousePosition(), camera);
    }

    for (int i = 0; i < clickCount; i++)
        {
            DrawRectangleLines(arrayPositions[i].x, arrayPositions[i].y, 100, 40, BLACK);
        }
}

int main(void)
{

    const int screenWidth = 1200;
    const int screenHeight = 800;

    Camera2D camera = {0};
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(60);              

    while (!WindowShouldClose())   
    {
         camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove()*0.1f));

         if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
 		{
 			int display = GetCurrentMonitor();
            
            if (IsWindowFullscreen())
            {
                SetWindowSize(screenWidth, screenHeight);
            }
            else
            {
                SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
            }

 			ToggleFullscreen();
 		}

        BeginDrawing();

            ClearBackground(RAYWHITE);
            
            BeginMode2D(camera);

            checkClick(camera);

            EndMode2D();

            

        EndDrawing();
    }

    CloseWindow();        

    return 0;
}