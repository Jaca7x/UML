
#include "src/lib/raylib.h"

int main(void)
{

    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(60);              

    while (!WindowShouldClose())   
    {
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

            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
            DrawText("Congrats! You created your first window!", 220, 200, 20, RED);
            DrawText("Congrats! You created your first window!", 220, 200, 20, RED);

        EndDrawing();
    }

    CloseWindow();        

    return 0;
}