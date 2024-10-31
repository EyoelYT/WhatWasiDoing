##
# froomf
#
# @file
# @version 0.1

## SHELL COMMAND = gcc main.cpp `sdl2-config --cflags --libs`

# Compiler Settings
CC := gcc
CFLAGS := $(shell sdl2-config --cflags)
LIBS := $(shell sdl2-config --libs) -lSDL2_ttf

# Source file and output executable
SRC := main.cpp
EXECUTABLE := froomf

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(EXECUTABLE)

# end
