#include <iostream>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



const char* getHomeDir() {
    return getenv("HOME");
}

char** readWaits(char* path, size_t* outLineCount) {
    void* fileContent = SDL_LoadFile(path, NULL);

    if (!fileContent) {
        printf("readWaits::fileContent: %s\n", fileContent);
        printf("readWaits::Error loading file: %s\n", SDL_GetError());
        return NULL;
    }

    char** matchingLines = (char**)malloc(100 * sizeof(char*));
    size_t count = 0;
    char* line = strtok((char*)fileContent, "\n");

    // Search for "WAIT" in each line
    while (line != NULL) {
        if (strstr(line, "WAIT")) {
            matchingLines[count++] = strdup(line);
            printf("\nreadWaits::MATCHING WAITS:\n%s\n", line);
        }
        line = strtok(NULL, "\n");
    }

    *outLineCount = count;
    return matchingLines;

    SDL_free(fileContent);
}

char* extractPath(const char* line) {
    const char* start = strchr(line, '"');
    if (!start) {
        return NULL; // no starting double quotes found
    }
    const char* end = strchr(start + 1, '"');
    if (!end) {
        return NULL; // no closing double quotes found
    }

    size_t len = end - start - 1;
    char* path = (char*)malloc(len + 1);
    if (!path) {
        perror("extractPath::Memory allocation error");
        exit(1);
    }
    strncpy(path, start + 1, len);
    path[len] = '\0';

    return path;
}

int main(int argc, char *argv[]) {
    const char* confFileName = "/.currTasks.conf";
    const int MAX_LINES_IN_FILE = 100;

    // Get the user's home dir
    const char* homeDir = getenv("HOME");
    if(!homeDir) {
        perror("main::Error getting home directory");
        return 1;
    }
    printf("\nmain::homeDir: %s\n\n", homeDir);

    // Get the full file path
    char confFilePath[1024]; // is this the same as char* confFilePath = (char*)malloc(1024) ??
    snprintf(confFilePath, sizeof(confFilePath), "%s%s", homeDir, confFileName);

    // Get config file vars
    FILE* file = fopen(confFilePath, "r");
    if (!file) {
        perror("main::Error opening file");
        printf("'%s' not found\n", confFilePath);
        return 1;
    }

    char* lines[MAX_LINES_IN_FILE]; // array of addresses to beginnings of chars
    size_t numLines = 0;

    char buffer[1024];

    // for every line from file into buffer
    while(fgets(buffer, sizeof(buffer), file)) {

        // if length of buffer > 1 and buffer has a '\n', allocate a '\0' at the end
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        // put contents of buffer into new memory, make lines[*char] point to start of buffer
        lines[numLines] = strdup(buffer);
        if (!lines[numLines]) {
           perror("main::Memory allocation error");
           fclose(file);
           return 1;
        }

        numLines++;
        if (numLines >= MAX_LINES_IN_FILE) {
            printf("main::Warning: Reached maximum number of lines in file: %d", MAX_LINES_IN_FILE);
            break;
        }
    }

    fclose(file);

    // Show the lines that are read
    printf("main::Lines read from %s:\n", confFilePath);
    for (size_t i = 0; i < numLines; ++i) {
        printf("%zu: %s\n", i + 1, lines[i]);
    }

    /* lines is an array of addresses, each that point to a char (which is a char string) */
    // extract the path strings from all the lines
    char* extractedPaths[MAX_LINES_IN_FILE];
    size_t numExtractedPaths = 0;

    for (size_t i = 0; i < numLines; ++i) {
        char* path = extractPath(lines[i]);
        if (path) {
            extractedPaths[numExtractedPaths++] = path;
        }
    }

    size_t matchingLinesCount;

    printf("\nmain::Initializing SDL\n");
    SDL_Init(0);

    SDL_Window* window = SDL_CreateWindow("SDL Window",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          680, 480,
                                          SDL_WINDOW_BORDERLESS);
    if(!window)
    {
        printf("main::Failed to create window\n");
        return -1;
    }

    SDL_Surface* window_surface = SDL_GetWindowSurface(window);

    if(!window_surface)
    {
        printf("main::Failed to get the surface from the window\n");
        return -1;
    }

    SDL_UpdateWindowSurface(window);

    printf("\nmain::Extracted paths:\n");
    for (size_t i = 0; i < numExtractedPaths; ++i) {
        printf("%zu: %s\n", i + 1, extractedPaths[i]);
        readWaits(extractedPaths[i], &matchingLinesCount);
    }
    printf("\nmain::Matches found: %lu\n", matchingLinesCount);

    SDL_Delay(5000);

    SDL_Quit();

    // free the allocated memory (the array if char*)
    for (size_t i = 0; i < numLines; ++i) {
        free(lines[i]);
    }
    printf("main::Exit\n");
    return 0;
}
