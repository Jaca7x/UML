# Makefile — RayUML Editor (build desktop, Windows/w64devkit)
#
# Uso:
#   make                    -> build release (game.exe)
#   make BUILD_MODE=DEBUG   -> build debug (símbolos, sem otimização)
#   make run                -> build + executa
#   make clean              -> remove artefatos de build
#
# Só recompila os .c que mudaram (ou cujos headers mudaram) desde o
# último build — bem mais rápido que recompilar tudo a cada save.

PROJECT_NAME ?= game
RAYLIB_PATH  ?= C:/raylib/raylib
W64DEVKIT    ?= C:/raylib/w64devkit
BUILD_MODE   ?= RELEASE

export PATH := $(W64DEVKIT)/bin:$(PATH)
CC = $(W64DEVKIT)/bin/gcc

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
LDLIBS  = -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -luser32

EXE = $(PROJECT_NAME).exe

.PHONY: all run clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) -MMD -MP

run: all
	-taskkill /F /IM $(EXE) /T >nul 2>&1
	./$(EXE)

clean:
	rm -rf $(OBJ_DIR) $(EXE)

-include $(DEPS)
