@echo off
:: 1. FORÇA o uso exclusivo da pasta bin da Raylib no topo do sistema
set PATH=C:\raylib\w64devkit\bin;%PATH%

:: 2. Configura os caminhos
set RAYLIB_PATH=C:/raylib/raylib

:: 3. Fecha o jogo se ele estiver aberto
taskkill /F /IM game.exe /T >nul 2>&1

:: 4. Compila o projeto (Note que agora uso apenas 'gcc' pois ele ja esta no PATH acima)
echo Compilando...
gcc main.c -o game.exe -Wall -std=c99 -DPLATFORM_DESKTOP -I. -I%RAYLIB_PATH%/src -L%RAYLIB_PATH%/src -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -luser32

:: 5. Se a compilação deu certo, roda o jogo
if %errorlevel% equ 0 (
    echo Sucesso! Abrindo jogo...
    game.exe
) else (
    echo.
    echo [ERRO DE COMPILACAO] Verifique o codigo acima.
)