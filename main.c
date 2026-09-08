#include "lib/raylib.h"
#include "src/include/renderer.h"
#include "src/include/editor.h"
#include "src/include/relations.h"
#include "src/include/ui.h"
#include "src/include/uifont.h"
#include "src/include/storage.h"
#include "src/include/history.h"
#include <math.h>

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    // Sem isso o Windows estica a janela inteira quando o display usa
    // escalonamento (125%, 150%), o que borra e engrossa todo o texto.
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);

    InitWindow(screenWidth, screenHeight, "RayUML Editor");
    LoadUiFont();

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};

    SetTargetFPS(60);

    // Arrasto do canvas com o botao esquerdo, iniciado no vazio
    bool draggingCanvas = false;

    while (!WindowShouldClose()) {

        int frameCursor = MOUSE_CURSOR_DEFAULT;
        Vector2 delta = GetMouseDelta();

        camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        // O esquerdo so arrasta o fundo se o clique comecou em area livre; em
        // cima de uma classe ele move a classe, e na interface pertence ao painel
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), camera);

            draggingCanvas = !IsMouseOverUi()
                          && !IsClassNameRequired()
                          && !IsPlacingClass()
                          && !IsRelationModeArmed()
                          && !IsConnectingRelation()
                          && GetClassIndexAt(worldPos) == -1;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) draggingCanvas = false;

        if (draggingCanvas) frameCursor = MOUSE_CURSOR_RESIZE_ALL;

        if (draggingCanvas || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }


        if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
        {
            ToggleFullscreenMode();
        }

        if (IsKeyPressed(KEY_Z) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            UndoHistory();
        }

        if (IsFileDropped())
        {
            FilePathList dropped = LoadDroppedFiles();

            if (dropped.count > 0)
            {
                bool loaded = LoadDiagram(dropped.paths[0]);
                ShowUiStatus(loaded ? "Diagrama carregado" : "Arquivo invalido", loaded);
            }

            UnloadDroppedFiles(dropped);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            
            BeginMode2D(camera);
                
                DrawWorldGrid(10000, 50.0f, LIGHTGRAY);
                UpdateAndDrawRelations(camera, &frameCursor);
                UpdateAndDrawBoxes(camera, &frameCursor);

            EndMode2D();

            DrawUi(&frameCursor);

            SetMouseCursor(frameCursor);
            
        EndDrawing();
    }

    UnloadUiFont();
    CloseWindow();
    return 0;
}