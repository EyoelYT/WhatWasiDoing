##
# froomf
#
# @file
# @version 0.1

## SHELL COMMAND = gcc main.cpp `sdl2-config --cflags --libs`

# Compiler Settings
CC := gcc
CFLAGS := $(shell sdl2-config --cflags)
LIBS := $(shell sdl2-config --libs)

# Source file and output executable
SRC := main.cpp
EXECUTABLE := froomf

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRC)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

clean:
	rm -f $(EXECUTABLE)

# end
