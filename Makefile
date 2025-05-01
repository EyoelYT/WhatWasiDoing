##
# froomf
#
# @file
# @version 0.1

# Compiler Settings
CC := gcc

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs) -lSDL2_ttf

BUILD_DIR  := build
SRC        := src/main.c
EXECUTABLE := $(BUILD_DIR)/froomf

ifeq ($(OS),Windows_NT)
    # [-mconsole | -DDEBUG_MODE]
    CCFLAGS := -O0 -Wall -Wextra -mconsole -DDEBUG_MODE
    LIBS     := $(SDL_LIBS)
else
    # [-DDEBUG_MODE]
    CCFLAGS := -O0 -Wall -Wextra
    LIBS     := $(SDL_LIBS)
endif

.PHONY: all clean

all: $(EXECUTABLE)

# Create build dir if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# After build dir, create output binary
$(EXECUTABLE): $(BUILD_DIR) $(SRC)
	$(CC) $(CCFLAGS) $(SDL_CFLAGS) -o $@ $(word 2,$^) $(LIBS)

clean:
	rm -v $(EXECUTABLE)

# end
