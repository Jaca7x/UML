#include "lib/raylib.h"
#include "src/include/renderer.h"
#include "src/include/editor.h"
#include <math.h>

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "RayUML Editor");

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        Vector2 delta = GetMouseDelta();
        camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode2D(camera);
                
                DrawWorldGrid(10000, 50.0f, LIGHTGRAY);
                UpdateAndDrawBoxes(camera);           
                
            EndMode2D();
            DrawText("UML Editor v1.0", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}