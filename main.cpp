#include <iostream>
#include <SDL.h>



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

int main(int argc, char *argv[]) {
    printf("Initializing SDL\n");
    SDL_Init(0);

    read("./data.txt");

    SDL_Quit();

    printf("Exit\n");
    return 0;
}
