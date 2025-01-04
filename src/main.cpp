#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef DEBUG_MODE
#define print_in_debug_mode(...) printf(__VA_ARGS__)
#else
#define print_in_debug_mode(...) (void(0))
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif

struct {
    unsigned char r = 64;
    unsigned char g = 128;
    unsigned char b = 64;
    unsigned char a = 240;
} BgColor;

void read_waits_from_targets(char* path, char* matching_lines[], int* curr_line_in_matching_lines) {
    void* file_content = SDL_LoadFile(path, NULL);

    if (!file_content) {
        print_in_debug_mode("readWaits::file_content: %s\n", (char*)file_content);
        print_in_debug_mode("readWaits::Error loading file: %s\n", SDL_GetError());
        return;
    }

    char* line = strtok((char*)file_content, "\n");

    // Search for "WAIT" in each line
    while (line != NULL) {
        if (strstr(line, "WAIT")) {
            matching_lines[*curr_line_in_matching_lines] = strdup(line);
            (*curr_line_in_matching_lines)++;
            print_in_debug_mode("\nreadWaits::MATCHING WAITS:\n%s\n", line);
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

char* extract_path(const char* line) {
    const char* start = strchr(line, '"');
    if (!start) {
        return NULL; // no starting double quotes found
    }
    const char* end = strchr(start + 1, '"');
    if (!end) {
        return NULL; // no closing double quotes found
    }

    int len = end - start - 1;
    char* path = (char*)malloc(len + 1);
    if (!path) {
        perror("extract_path::Memory allocation error");
        exit(1);
    }
    strncpy(path, start + 1, len);
    path[len] = '\0';

    return path;
}

int read_config_file(const char* conf_file_path, const int MAX_LINES_IN_FILE, char* lines[]) {
    // Get config file vars
    FILE* file = fopen(conf_file_path, "r");
    if (!file) {
        perror("main::Error opening file");
        print_in_debug_mode("'%s' not found\n", conf_file_path);
        return 1;
    }

    int num_lines = 0;

    char buffer[1024];

    // for every line from configuration file into buffer
    while (fgets(buffer, sizeof(buffer), file)) {

        // if length of buffer > 1 and buffer has a '\n', allocate a '\0' at the end
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        // put contents of buffer into new memory, make lines[*char] point to start of buffer
        lines[num_lines] = strdup(buffer);
        if (!lines[num_lines]) {
            perror("main::Memory allocation error");
            fclose(file);
            return 1;
        }

        num_lines++;
        if (num_lines >= MAX_LINES_IN_FILE) {
            print_in_debug_mode("main::Warning: Reached maximum number of lines in file: %d",
                                MAX_LINES_IN_FILE);
            break;
        }
    }

    // Show the lines that are read
    print_in_debug_mode("main::Lines read from %s:\n", conf_file_path);
    for (int i = 0; i < num_lines; ++i) {
        print_in_debug_mode("%d: %s\n", i + 1, lines[i]);
    }

    fclose(file);

    return num_lines;
}

void get_target_paths(char* extracted_paths[], int* num_extracted_paths,
                      int num_lines_in_config_file, char* lines[]) {
    for (int i = 0; i < num_lines_in_config_file; ++i) {
        char* path = extract_path(lines[i]);
        if (path) {
            extracted_paths[(*num_extracted_paths)++] = path;
        }
    }
}

void get_matching_lines_from_targets(char* extracted_paths[], int num_extracted_paths,
                                     char* matching_lines[], int* matching_lines_count) {
    print_in_debug_mode("\nmain::Extracted paths:\n");
    for (int i = 0; i < num_extracted_paths; ++i) {
        print_in_debug_mode("\033[33m%d: %s\033[0m\n", i + 1, extracted_paths[i]);
        read_waits_from_targets(extracted_paths[i], matching_lines, matching_lines_count);
    }
    print_in_debug_mode("\nmain::Matches found: %d\n", *matching_lines_count);

    for (int i = 0; i < *matching_lines_count; ++i) {
        print_in_debug_mode("main::PRINT MATCHING LINES: %s\n", matching_lines[i]);
    }
}

int main(int argc, char* argv[]) {
    const char* conf_file_name = "/.currTasks.conf";
    const int MAX_LINES_IN_FILE = 100;
    char* keyword = (char*)"WAIT";

    char* lines[MAX_LINES_IN_FILE]; // array of addresses to beginnings of chars

    char conf_file_path[1024];
    int num_lines_in_config_file;

    char* extracted_paths[MAX_LINES_IN_FILE];
    int num_extracted_paths;

    char** matching_lines;
    int matching_lines_count;

    // Get the user's home dir
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        perror("main::Error getting home directory");
        return 1;
    }
    // Get the full file path to the config file (join the strings)
    snprintf(conf_file_path, sizeof(conf_file_path), "%s%s", home_dir, conf_file_name);
    num_lines_in_config_file = read_config_file(conf_file_path, MAX_LINES_IN_FILE, lines);

    // extract the path strings from all the lines
    num_extracted_paths = 0;
    get_target_paths(extracted_paths, &num_extracted_paths, num_lines_in_config_file, lines);

    matching_lines = (char**)malloc(100 * sizeof(char*));
    matching_lines_count = 0;
    get_matching_lines_from_targets(extracted_paths, num_extracted_paths, matching_lines,
                                    &matching_lines_count);

    // SDL /////////////////////////////////////////////////////////
    print_in_debug_mode("\nmain::Initializing SDL_ttf\n");
    if (TTF_Init() == -1) {
        print_in_debug_mode("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    print_in_debug_mode("\nmain::Loading font\n");
#ifdef _WIN32
    const char* font_path = "C:\\Users\\Eyu\\Projects\\probe\\nerd-fonts\\patched-"
                            "fonts\\Iosevka\\IosevkaNerdFont-Regular.ttf";
#else
    const char* font_path = "/usr/share/fonts/TTF/IosevkaNerdFont-Regular.ttf";
#endif
    int font_size = 36;
    TTF_Font* font = TTF_OpenFont(font_path, font_size);
    if (!font) {
        print_in_debug_mode("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    print_in_debug_mode("\nmain::Initializing SDL\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        print_in_debug_mode("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL Window", 10, 1550, 2500, 50, SDL_WINDOW_RESIZABLE);
    if (!window) {
        print_in_debug_mode("Failed to create window\n");
        return -1;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        if (wmInfo.subsystem == SDL_SYSWM_X11) {
#ifdef linux
            Display* display = wmInfo.info.x11.display;
            Window x11Window = wmInfo.info.x11.window;

            // Get the _NET_WM_STATE and _NET_WM_STATE_ABOVE atoms
            Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
            Atom wmStateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

            // Prepare the event to set the window as "always on top"
            XEvent event;
            memset(&event, 0, sizeof(event));
            event.xclient.type = ClientMessage;
            event.xclient.window = x11Window;
            event.xclient.message_type = wmState;
            event.xclient.format = 32;
            event.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
            event.xclient.data.l[1] = wmStateAbove;
            event.xclient.data.l[2] = 0;
            event.xclient.data.l[3] = 1;
            event.xclient.data.l[4] = 0;

            // Send the event to the X server
            XSendEvent(display, DefaultRootWindow(display), False,
                       SubstructureNotifyMask | SubstructureRedirectMask, &event);
            XFlush(display);
#endif
        }
#ifdef _WIN32
        else if (wmInfo.subsystem == SDL_SYSWM_WINDOWS) {
            // Get the window handle
            HWND hwnd = wmInfo.info.win.window;

            // Set to always on top
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
#endif
        else {
            print_in_debug_mode("SDL_GetWindowWMInfo Error: %s\n", SDL_GetError());
        }
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        print_in_debug_mode("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
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
                    BgColor.r -= 16;
                } else if (event.key.keysym.sym == SDLK_g && (event.key.keysym.mod & KMOD_SHIFT)) {
                    BgColor.g -= 16;
                } else if (event.key.keysym.sym == SDLK_b && (event.key.keysym.mod & KMOD_SHIFT)) {
                    BgColor.b -= 16;
                } else if (event.key.keysym.sym == SDLK_a && (event.key.keysym.mod & KMOD_SHIFT)) {
                    BgColor.a -= 16;
                } else {
                    switch (event.key.keysym.sym) {
                    case SDLK_r:
                        BgColor.r += 16;
                        break;
                    case SDLK_g:
                        BgColor.g += 16;
                        break;
                    case SDLK_b:
                        BgColor.b += 16;
                        break;
                    case SDLK_a:
                        BgColor.a += 16;
                        break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, BgColor.r, BgColor.g, BgColor.b, BgColor.a);
        print_in_debug_mode("BG Colors:\n    r = %u\n    g = %u\n    b = %u\n    a = %u\n",
                            BgColor.r, BgColor.g, BgColor.b, BgColor.a);
        SDL_RenderClear(renderer); // Clear the renderer buffer

        int y_offset = 0;
        for (int i = 0; i < matching_lines_count; i++) {

            SDL_Color text_color = {255, 255, 255, 255};
            SDL_Surface* text_surface;
            if (i == 0) {
                char first_line_text[256];
                const char* prefix = "Current Task: ";
                trim_keyword_prefix(matching_lines[0], keyword);
                snprintf(first_line_text, sizeof(first_line_text), "%s%s", prefix,
                         matching_lines[0]);
                text_surface = TTF_RenderText_Solid(font, first_line_text, text_color);
            } else {
                text_surface = TTF_RenderText_Solid(font, matching_lines[i], text_color);
            }
            if (!text_surface) {
                fprintf(stderr, "Unable to render text! TTF_Error: %s\n", TTF_GetError());
                continue;
            }

            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            SDL_FreeSurface(text_surface); // No longer needed
            if (!text_texture) {
                fprintf(stderr, "Failed to create texture! SDL_Error: %s\n", SDL_GetError());
                continue;
            }

            SDL_Rect src_Rect = {0, 0, text_surface->w, text_surface->h};
            SDL_Rect dstRect = {0, y_offset, text_surface->w, text_surface->h};
            SDL_RenderCopy(renderer, text_texture, &src_Rect, &dstRect);
            SDL_DestroyTexture(text_texture);

            y_offset += text_surface->h;
        }
        SDL_RenderPresent(renderer); // Update screen

        SDL_Delay(1000);

        matching_lines_count = 0;
        get_matching_lines_from_targets(extracted_paths, num_extracted_paths, matching_lines,
                                        &matching_lines_count);
    }

    print_in_debug_mode("\nmain::Destroying Renderer\n");
    SDL_DestroyRenderer(renderer);
    print_in_debug_mode("\nmain::Destroying Window\n");
    SDL_DestroyWindow(window);
    print_in_debug_mode("\nmain::Closing SDL_ttf\n");
    TTF_CloseFont(font);
    TTF_Quit();
    print_in_debug_mode("\nmain::Quitting SDL\n");
    SDL_Quit();

    // free the allocated memory (the array if char*)
    for (int i = 0; i < num_lines_in_config_file; ++i) {
        free(lines[i]);
    }
    free(matching_lines);
    print_in_debug_mode("\nmain::Exit\n");
    return 0;
}
