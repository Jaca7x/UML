# Makefile — RayUML Editor (build desktop, Windows/w64devkit)
#
# Uso:
#   make                    -> build release (game.exe)
#   make BUILD_MODE=DEBUG   -> build debug (símbolos, sem otimização)
#   make run                -> build + executa
#   make test               -> compila e roda os testes (sem abrir janela)
#   make clean              -> remove artefatos de build
#
# Só recompila os .c que mudaram (ou cujos headers mudaram) desde o
# último build — bem mais rápido que recompilar tudo a cada save.

PROJECT_NAME ?= game
BUILD_MODE   ?= RELEASE

# O editor e desenvolvido no Windows com w64devkit, mas os testes tambem
# rodam na CI, que e Linux: la o compilador e o do sistema e a raylib pede
# outro conjunto de bibliotecas.
ifeq ($(OS),Windows_NT)
    RAYLIB_PATH ?= C:/raylib/raylib
    W64DEVKIT   ?= C:/raylib/w64devkit

    # O gcc do w64devkit precisa achar o "as" que veio com ele: com um MinGW
    # global no PATH, a montagem falha com "bad register name '%rbp'"
    export PATH := $(W64DEVKIT)/bin:$(PATH)

    CC       = $(W64DEVKIT)/bin/gcc
    LDLIBS   = -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -luser32
    EXE      = $(PROJECT_NAME).exe
    TEST_EXE = build/testes.exe
else
    RAYLIB_PATH ?= /usr/local

    CC       = gcc
    LDLIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    EXE      = $(PROJECT_NAME)
    TEST_EXE = build/testes
endif

SRC     = main.c $(wildcard src/modules/*.c)
OBJ_DIR = build/obj
OBJS    = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEPS    = $(OBJS:.o=.d)

CFLAGS  = -std=c99 -Wall -DPLATFORM_DESKTOP
CFLAGS += -I. -Isrc/include -I$(RAYLIB_PATH)/src

ifeq ($(BUILD_MODE),DEBUG)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O1
endif

LDFLAGS = -L$(RAYLIB_PATH)/src

.PHONY: all run clean test

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) -MMD -MP

run: all
	-taskkill /F /IM $(EXE) /T >nul 2>&1
	./$(EXE)

# Testes: rodam sem abrir janela, entao servem tambem para CI. Compilam os
# modulos direto, sem o main.c do editor, que tem o proprio main().
TEST_SRC = $(wildcard tests/*.c) $(wildcard src/modules/*.c)

test: $(TEST_EXE)
	./$(TEST_EXE)

$(TEST_EXE): $(TEST_SRC) $(wildcard tests/*.h) $(wildcard src/include/*.h)
	@mkdir -p build
	$(CC) $(TEST_SRC) -o $@ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(OBJ_DIR) $(EXE) $(TEST_EXE) build/teste

-include $(DEPS)
