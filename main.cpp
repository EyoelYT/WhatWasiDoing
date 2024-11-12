#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



const char* getHomeDir() {
    return getenv("HOME");
}

void readWaits(char* path, size_t* outLineCount, char** matchingLines, size_t* currLineInML) {
    void* fileContent = SDL_LoadFile(path, NULL);

    if (!fileContent) {
        printf("readWaits::fileContent: %s\n", (char*)fileContent);
        printf("readWaits::Error loading file: %s\n", SDL_GetError());
        return;
    }

    char* line = strtok((char*)fileContent, "\n");

    // Search for "WAIT" in each line
    while (line != NULL) {
        if (strstr(line, "WAIT")) {
            matchingLines[*currLineInML] = strdup(line);
            (*currLineInML)++;
            printf("\nreadWaits::MATCHING WAITS:\n%s\n", line);
        }
        line = strtok(NULL, "\n");
    }
    SDL_free(fileContent);

    *outLineCount = *currLineInML;
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
    if (!homeDir) {
        perror("main::Error getting home directory");
        return 1;
    }
    printf("\nmain::homeDir: %s\n\n", homeDir);

    // Get the full file path
    char confFilePath[1024];
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

    // for every line from configuration file into buffer
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

    char** matchingLines = (char**)malloc(100 * sizeof(char*));

    printf("\nmain::Extracted paths:\n");
    size_t matchingLinesCount = 0;
    size_t currLineInML = 0;
    for (size_t i = 0; i < numExtractedPaths; ++i) {
        printf("\033[33m%zu: %s\033[0m\n", i + 1, extractedPaths[i]);
        readWaits(extractedPaths[i], &matchingLinesCount, matchingLines, &currLineInML);
    }
    printf("\nmain::Matches found: %lu\n", matchingLinesCount);

    for (size_t i = 0; i < matchingLinesCount; ++i) {
        printf("main::PRINT MATCHING LINES: %s\n", matchingLines[i]);
    }
    // SDL /////////////////////////////////////////////////////////
    printf("\nmain::Initializing SDL_ttf\n");
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    printf("\nmain::Loading font\n");
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/TTF/IosevkaNerdFont-Regular.ttf", 24);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    printf("\nmain::Initializing SDL\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          680, 480, 0); // SDL_WINDOW_BORDERLESS
    if (!window)
    {
        printf("Failed to create window\n");
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    running = false;
                }
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black bg
        SDL_RenderClear(renderer);                      // Clear the renderer buffer

        int yOffset = 0;
        for (size_t i = 0; i < matchingLinesCount; i++) {

            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, matchingLines[i], textColor);
            if (!textSurface) {
                fprintf(stderr, "Unable to render text! TTF_Error: %s\n", TTF_GetError());
                continue;
            }

            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_FreeSurface(textSurface); // No longer needed
            if (!textTexture) {
                fprintf(stderr, "Failed to create texture! SDL_Error: %s\n", SDL_GetError());
                continue;
            }

            SDL_Rect srcRect = {0, 0, textSurface->w, textSurface->h};
            SDL_Rect dstRect = {0, yOffset, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, &srcRect, &dstRect);
            SDL_DestroyTexture(textTexture);

            yOffset += textSurface->h;
        }
        SDL_RenderPresent(renderer); // Update screen

        SDL_Delay(1000);
    }

    printf("\nmain::Destroying Renderer\n");
    SDL_DestroyRenderer(renderer);
    printf("\nmain::Destroying Window\n");
    SDL_DestroyWindow(window);
    printf("\nmain::Closing SDL_ttf\n");
    TTF_CloseFont(font);
    TTF_Quit();
    printf("\nmain::Quitting SDL\n");
    SDL_Quit();

    // free the allocated memory (the array if char*)
    for (size_t i = 0; i < numLines; ++i) {
        free(lines[i]);
    }
    free(matchingLines);
    printf("\nmain::Exit\n");
    return 0;
}
