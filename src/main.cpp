#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EXTRACTED_PATHS 90
#define MAX_KEYWORDS 10
#define MAX_LINES_IN_CONFIG_FILE 100
#define MAX_MATCHING_LINES_CAP 150
#define MAX_STRING_LENGTH_CAP 512

#ifdef DEBUG_MODE
    #define debug_p(...) printf(__VA_ARGS__)
#else
    #define debug_p(...) (void(0))
#endif

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

void read_keyword_lines_from_filepath(char* path, char** matching_lines, size_t* curr_line_in_matching_lines, char** keywords, size_t num_keywords) {
    void* file_content = SDL_LoadFile(path, NULL);

    if (!file_content) {
        debug_p("readWaits::Error loading file: %s\n", SDL_GetError());
        debug_p("readWaits::file_content: %s\n", (char*)file_content);
        return;
    }

    char* line = strtok((char*)file_content, "\n");

    // Search for the keywords in each line
    while (line != NULL && *curr_line_in_matching_lines < MAX_MATCHING_LINES_CAP) {
        for (size_t i = 0; i < num_keywords; i++) {
            if (strstr(line, keywords[i])) {
                strcpy(matching_lines[*curr_line_in_matching_lines], line);
                (*curr_line_in_matching_lines)++;
                debug_p("\nreadWaits::MATCHING WAITS:\n%s\n", line);
            }
        }
        line = strtok(NULL, "\n");
    }
    SDL_free(file_content);
}

void trim_keyword_prefix(char* line, char* keyword) {
    if (!line)
        return;

    char* pline = line;

    if (*pline == '*') {
        while (*pline == '*') {
            pline++;
        }
        pline++; // Extra space
    }

    size_t min = strlen(keyword);
    if (strncmp(pline, keyword, min) == 0) {
        pline += min;
        pline++; // Extra space
    }

    memmove(line, pline, strlen(pline) + 1);
}

FILE* create_demo_config_file(const char* conf_file_path) {
    FILE* file = fopen(conf_file_path, "wb");
    if (!file) {
        fprintf(stderr, "create_demo_config_file::Couldn't create demo file '%s': %s\n", conf_file_path, strerror(errno));
        return NULL;
    }

#ifdef _WIN32
    char generated_file_content[] = "file = \"C:\\Users\\USER\\todos.org\"\n"
                                    "keyword = \"TODO\"\n";
#else
    char generated_file_content[] = "file = \"/home/USER/todos.org\"\n"
                                    "keyword = \"TODO\"\n";
#endif
    size_t generated_file_content_size = sizeof(generated_file_content);

    fwrite(generated_file_content, sizeof generated_file_content[0], generated_file_content_size, file);

    fclose(file);
    return fopen(conf_file_path, "r");
}

int read_config_file(const char* conf_file_path, char** config_file_lines) {
    // Get config file vars
    FILE* file = fopen(conf_file_path, "r");
    if (!file) {
        debug_p("read_config_file::File doesn't exist. Creating demo file.");
        debug_p("'%s' not found\n", conf_file_path);
        file = create_demo_config_file(conf_file_path);
        if (!file) {
            perror("read_config_file::Unable to create file");
            exit(1);
        }
    }

    size_t curr_line = 0;

    char buffer[MAX_STRING_LENGTH_CAP];

    // for every line from configuration file into buffer
    while (fgets(buffer, sizeof(buffer), file)) {

        // if length of buffer > 1 and buffer has a '\n', allocate a '\0' at the end
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        strcpy(config_file_lines[curr_line], buffer);
        curr_line++;

        if (curr_line >= MAX_LINES_IN_CONFIG_FILE) {
            debug_p("main::Warning: Reached maximum number of lines in file: %zu", MAX_LINES_IN_CONFIG_FILE);
            break;
        }
    }

    // Show the lines that are read
    debug_p("\nmain::Lines read from %s:\n", conf_file_path);
    for (size_t i = 0; i < curr_line; i++) {
        debug_p("%zu: %s\n", i + 1, config_file_lines[i]);
    }

    fclose(file);

    // return the number of lines
    return curr_line;
}

size_t extract_config_file_values(char* key, char** destination_array, size_t destination_array_len, char** source_array, size_t source_array_len) {

    size_t num_keys = 0;
    size_t j = 0; // j points into the destination array
    for (size_t i = 0; i < source_array_len; i++) {
        char buf[MAX_STRING_LENGTH_CAP];
        char* start = strchr(source_array[i], '"'); // first quote
        if (!start)
            continue;
        start++;                        // skip the quote
        char* end = strchr(start, '"'); // matching second quote
        if (!end)
            continue;

        size_t len = end - start;
        strncpy(buf, start, len); // copy string from 'start' to 'end' into buf
        buf[len] = '\0';

        // copy the substring within quotes into the destination array
        if (j < destination_array_len) {
            if (strstr(source_array[i], key) != NULL) {
                strcpy(destination_array[j], buf);
                num_keys++;
                j++;
            }
        }
    }
    return num_keys;
}

int main(int argc, char* argv[]) {

    char* keywords[MAX_KEYWORDS];
    for (size_t i = 0; i < MAX_KEYWORDS; i++) {
        keywords[i] = (char*)malloc(MAX_STRING_LENGTH_CAP);
    }

    char* extracted_paths[MAX_EXTRACTED_PATHS];
    for (size_t i = 0; i < MAX_EXTRACTED_PATHS; i++) {
        extracted_paths[i] = (char*)malloc(MAX_STRING_LENGTH_CAP);
    }

    char* config_file_lines[MAX_LINES_IN_CONFIG_FILE];
    for (size_t i = 0; i < MAX_LINES_IN_CONFIG_FILE; i++) {
        config_file_lines[i] = (char*)malloc(MAX_STRING_LENGTH_CAP);
    }

    char* matching_lines[MAX_MATCHING_LINES_CAP];
    for (size_t i = 0; i < MAX_MATCHING_LINES_CAP; i++) {
        matching_lines[i] = (char*)malloc(MAX_STRING_LENGTH_CAP);
    }

    Color bg_color = {24, 128, 64, 240};
    size_t num_keywords;
    size_t num_extracted_paths;
    size_t num_lines_in_config_file;
    size_t curr_line_in_matching_lines;
    char conf_file_path[MAX_STRING_LENGTH_CAP];
    const char* title = "WhatWasiDoing";
#ifdef _WIN32
    const char* conf_file_name = "\\.currTasks.conf";
#else
    const char* conf_file_name = "/.currTasks.conf";
#endif

    // Get the user's home dir
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        perror("main::Error getting home directory");
        return 1;
    }
    // Get the full file path to the config file (join the strings)
    snprintf(conf_file_path, sizeof(conf_file_path), "%s%s", home_dir, conf_file_name);
    num_lines_in_config_file = read_config_file(conf_file_path, config_file_lines);

    num_extracted_paths = extract_config_file_values((char*)"file", extracted_paths, MAX_EXTRACTED_PATHS, config_file_lines, num_lines_in_config_file);
    num_keywords = extract_config_file_values((char*)"keyword", keywords, MAX_KEYWORDS, config_file_lines, num_lines_in_config_file);

    curr_line_in_matching_lines = 0;
    for (size_t i = 0; i < num_extracted_paths; i++) {
        debug_p("\033[33m%zu: %s\033[0m\n", i + 1, extracted_paths[i]);
        read_keyword_lines_from_filepath(extracted_paths[i], matching_lines, &curr_line_in_matching_lines, keywords, num_keywords);
    }

    // SDL /////////////////////////////////////////////////////////
    debug_p("\nmain::Initializing SDL_ttf\n");
    if (TTF_Init() == -1) {
        debug_p("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    debug_p("\nmain::Loading font\n");
#ifdef _WIN32
    const char* font_path = "C:\\Users\\Eyu\\Projects\\probe\\nerd-fonts\\patched-fonts\\Iosevka\\IosevkaNerdFont-Regular.ttf";
#else
    const char* font_path = "/usr/share/fonts/adobe-source-code-pro/SourceCodePro-Regular.otf";
#endif
    int font_size = 36;
    TTF_Font* font = TTF_OpenFont(font_path, font_size);
    if (!font) {
        debug_p("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    debug_p("\nmain::Initializing SDL\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        debug_p("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    int displayIndex = 0;
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(displayIndex, &mode);

    int res_x = 10;
    int width = mode.w - 60;

    int res_y = mode.h - 50;
    int height = 50;

    Uint32 sdl_window_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
    SDL_Window* window = SDL_CreateWindow(title, res_x, res_y, width, height, sdl_window_flags);
    SDL_bool bordered = SDL_FALSE;
    SDL_bool resizable = SDL_FALSE;
    SDL_bool always_on_top = SDL_TRUE;

    if (!window) {
        debug_p("Failed to create window\n");
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        debug_p("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
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
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_r && (event.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.r -= 16;
                } else if (event.key.keysym.sym == SDLK_g && (event.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.g -= 16;
                } else if (event.key.keysym.sym == SDLK_b && (event.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.b -= 16;
                } else if (event.key.keysym.sym == SDLK_a && (event.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.a -= 16;
                } else {
                    switch (event.key.keysym.sym) {
                    case SDLK_r:
                        bg_color.r += 16;
                        break;
                    case SDLK_g:
                        bg_color.g += 16;
                        break;
                    case SDLK_b:
                        bg_color.b += 16;
                        break;
                    case SDLK_a:
                        bg_color.a += 16;
                        break;
                    case SDLK_t:
                        // store current size and position
                        int current_width, current_height, current_x, current_y;
                        SDL_GetWindowSize(window, &current_width, &current_height);
                        SDL_GetWindowPosition(window, &current_x, &current_y);

                        if (bordered == SDL_FALSE) {
                            bordered = SDL_TRUE;
                        } else {
                            bordered = SDL_FALSE;
                        }
                        if (resizable == SDL_FALSE) {
                            resizable = SDL_TRUE;
                        } else {
                            resizable = SDL_FALSE;
                        }

                        SDL_SetWindowBordered(window, bordered);
                        SDL_SetWindowResizable(window, resizable);
                        // restore window from stored size and position to compensate for wierd SDL behavior
                        SDL_SetWindowSize(window, current_width, current_height);
                        SDL_SetWindowPosition(window, current_x, current_y);
                        break;
                    case SDLK_z:
                        SDL_SetWindowPosition(window, res_x, res_y);
                        SDL_SetWindowSize(window, width, height);
                        break;
                    case SDLK_p:
                        if (always_on_top == SDL_FALSE) {
                            always_on_top = SDL_TRUE;
                        } else {
                            always_on_top = SDL_FALSE;
                        }
                        SDL_SetWindowAlwaysOnTop(window, always_on_top);
                        break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        debug_p("BG Colors:\n\tr = %u\n\tg = %u\n\tb = %u\n\ta = %u\n", bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderClear(renderer); // Clear the renderer buffer

        int y_offset = 0;
        for (size_t i = 0; i < curr_line_in_matching_lines; i++) {

            SDL_Color text_color = {255, 255, 255, 255};
            SDL_Surface* text_surface;
            if (i == 0) {
                char first_line_text[256];
                const char* prefix = "Current Task: ";

                // trim out the keywords
                for (size_t i = 0; i < num_keywords; i++) {
                    trim_keyword_prefix(matching_lines[0], keywords[i]);
                }
                snprintf(first_line_text, sizeof(first_line_text), "%s%s", prefix, matching_lines[0]);
                text_surface = TTF_RenderText_Blended(font, first_line_text, text_color);
            } else {
                text_surface = TTF_RenderText_Blended(font, matching_lines[i], text_color);
            }
            if (!text_surface) {
                fprintf(stderr, "Unable to render text! TTF_Error: %s\n", TTF_GetError());
                continue;
            }

            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            int w = text_surface->w;
            int h = text_surface->h;
            SDL_FreeSurface(text_surface); // No longer needed
            if (!text_texture) {
                fprintf(stderr, "Failed to create texture! SDL_Error: %s\n", SDL_GetError());
                continue;
            }

            SDL_Rect src_rect = {0, 0, w, h};
            SDL_Rect dst_rect = {0, y_offset, w, h};
            SDL_RenderCopy(renderer, text_texture, &src_rect, &dst_rect);
            SDL_DestroyTexture(text_texture);

            y_offset += h;
        }
        SDL_RenderPresent(renderer); // Update screen

        SDL_Delay(256);

        curr_line_in_matching_lines = 0;
        for (size_t i = 0; i < num_extracted_paths; i++) {
            debug_p("\033[33m%zu: %s\033[0m\n", i + 1, extracted_paths[i]);
            read_keyword_lines_from_filepath(extracted_paths[i], matching_lines, &curr_line_in_matching_lines, keywords, num_keywords);
        }
    }

    debug_p("\nmain::Destroying Renderer\n");
    SDL_DestroyRenderer(renderer);

    debug_p("\nmain::Destroying Window\n");
    SDL_DestroyWindow(window);

    debug_p("\nmain::Closing SDL_ttf\n");
    TTF_CloseFont(font);
    TTF_Quit();

    debug_p("\nmain::Quitting SDL\n");
    SDL_Quit();

    for (size_t i = 0; i < num_lines_in_config_file; i++) {
        free(config_file_lines[i]);
    }

    for (size_t i = 0; i < num_lines_in_config_file; i++) {
        free(matching_lines[i]);
    }

    for (size_t i = 0; i < num_extracted_paths; i++) {
        free(extracted_paths[i]);
    }

    for (size_t i = 0; i < num_keywords; i++) {
        free(keywords[i]);
    }

    debug_p("\nmain::Exit\n");
    return 0;
}
