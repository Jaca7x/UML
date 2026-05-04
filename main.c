#include "lib/raylib.h"
#include "src/include/renderer.h"
#include "src/include/editor.h"
#include "src/include/ui.h"
#include <math.h>

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "RayUML Editor");

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};

    Rectangle ui = {10, 10, 100, 50};
    bool isDraw = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        int frameCursor = MOUSE_CURSOR_DEFAULT;
        Vector2 delta = GetMouseDelta();
        
        camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }

        
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
                
                DrawWorldGrid(10000, 50.0f, LIGHTGRAY);
                UpdateAndDrawBoxes(camera, &frameCursor);
                
            EndMode2D();

            DrawUi(ui, camera, &frameCursor);

            SetMouseCursor(frameCursor);
            
        EndDrawing();
    }

    CloseWindow();
    return 0;
}