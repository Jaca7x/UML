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
    
    
    camera.zoom = 1.0f;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(60);              

    while (!WindowShouldClose())   
    {
        Vector2 mousePosition = GetMousePosition();
        Vector2 delta = GetMouseDelta();    

         camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove()*0.1f));

         

         if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) 
         {
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
         }

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

            for (int i = -10000; i <= 10000; i += 50.0f)
            {
                DrawLine(i, -10000, i, 10000, LIGHTGRAY);
            }
            
            for (int i = -10000000; i <= 10000000; i += 50.0f)
            {
                DrawLine(-10000000, i, 10000000, i, LIGHTGRAY);
            }

            checkClick(camera);

            EndMode2D();

            DrawText(TextFormat("Pos: %f", mousePosition.x), 10, 10, 18, BLACK);

        EndDrawing();
    }

    CloseWindow();        

    return 0;
}