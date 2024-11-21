##
# froomf
#
# @file
# @version 0.1

## SHELL COMMAND = g++ main.cpp `sdl2-config --cflags --libs`

# Compiler Settings
CXX := g++
CXXFLAGS := -g -O0 -Wall -Wextra
SDL_CFLAGS := $(shell sdl2-config --cflags)
LIBS := $(shell sdl2-config --libs) -lSDL2_ttf

# Source file and output executable
SRC := main.cpp
EXECUTABLE := froomf

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRC)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(EXECUTABLE)

# end
