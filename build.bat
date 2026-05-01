@echo off

set PATH=C:\raylib\w64devkit\bin;%PATH%

set RAYLIB_PATH=C:/raylib/raylib

taskkill /F /IM game.exe /T >nul 2>&1

echo Compilando RayUML...

gcc main.c src/modules/*.c  -o game.exe ^
    -Wall -std=c99 -DPLATFORM_DESKTOP ^
    -I. -I./src/include -I%RAYLIB_PATH%/src ^
    -L%RAYLIB_PATH%/src ^
    -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -luser32

if %errorlevel% equ 0 (
    cls
    echo [SISTEMA] Compilado com sucesso! Abrindo...
    game.exe
) else (
    echo.
    echo [SISTEMA] ERRO DE COMPILACAO. Verifique o codigo acima.
)