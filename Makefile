# Caminhos explícitos para evitar conflito entre os 3 GCCs
RAYLIB_PATH = C:/raylib/raylib
COMPILER_PATH = C:/raylib/w64devkit/bin

# Força o uso do GCC que veio com a Raylib
CC = $(COMPILER_PATH)/gcc

PROJECT_NAME = game.exe
SRC = main.c
    
# Configurações de compilação
CFLAGS = -Wall -std=c99 -DPLATFORM_DESKTOP -I. -I$(RAYLIB_PATH)/src
LDFLAGS = -L$(RAYLIB_PATH)/src
LDLIBS = -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -luser32

all:
	$(CC) $(SRC) -o $(PROJECT_NAME) $(CFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	del *.exe *.o