#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



const char* getHomeDir() {
    return getenv("HOME");
}

void read(char* path) {
    size_t fileSize = sizeof(path);
    void* fileContent = SDL_LoadFile(path, &fileSize);

    if (!fileContent) {
        printf("File Content: %s\n", fileContent);
        printf("Error loading file: %s\n", SDL_GetError());
        return;
    }

    size_t size;
    SDL_RWops* context = SDL_RWFromFile(path, "r");
    if (context) {
        size = SDL_RWsize(context);
        SDL_RWclose(context);
    } else {
        printf("Error opening file for calculation");
        SDL_free(context);
        return;
    }

    printf("Loaded fileSize: %lu\n", fileSize);
    printf("Loaded size:     %lu\n", size);
    printf("File Content:    %s", fileContent);

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
        perror("Memory allocation error");
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
        perror("Error getting home directory");
        return 1;
    }
    printf("homeDir: %s\n", homeDir);

    // Get the full file path
    char confFilePath[1024]; // is this the same as char* confFilePath = (char*)malloc(1024) ??
    snprintf(confFilePath, sizeof(confFilePath), "%s%s", homeDir, confFileName);

    // Get config file vars
    FILE* file = fopen(confFilePath, "r");
    if (!file) {
        perror("Error opening file");
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
           perror("Memory allocation error");
           fclose(file);
           return 1;
        }

        numLines++;
        if (numLines >= MAX_LINES_IN_FILE) {
            printf("Warning: Reached maximum number of lines in file: %d", MAX_LINES_IN_FILE);
            break;
        }
    }

    fclose(file);

    // Show the lines that are read
    printf("Lines read from %s:\n", confFilePath);
    for (size_t i = 0; i < numLines; ++i) {
        printf("%zu: %s\n", i + 1, lines[i]);
    }

    /* lines is an array of addresses, each that point to a char (which is a char string) */
    // extract the path strings from all the lines
    size_t numberOfLines = sizeof(lines) / sizeof(lines[0]);
    char* extractedPaths[MAX_LINES_IN_FILE];
    size_t numExtractedPaths = 0;

    for (size_t i = 0; i < numLines; ++i) {
        char* path = extractPath(lines[i]);
        if (path) {
            extractedPaths[numExtractedPaths++] = path;
        }
    }

    printf("Extracted paths:\n");
    for (size_t i = 0; i < numExtractedPaths; ++i) { // TODO: i++ -> ++i
        printf("%zu: %s\n", i + 1, extractedPaths[i]);
    }

    printf("Initializing SDL\n");
    SDL_Init(0);

    char data[] = "./data.txt";
    read(data);

    SDL_Quit();

    // free the allocated memory (the array if char*)
    for (size_t i = 0; i < numLines; ++i) {
        free(lines[i]);
    }
    printf("Exit\n");
    return 0;
}
