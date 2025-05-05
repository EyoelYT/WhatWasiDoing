#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CYN "\x1B[36m"
#define YEL "\x1B[33m"
#define RESET "\x1B[0m"

#define MAX_TARGET_PATHS 90
#define MAX_KEYWORDS 10
#define MAX_LINES_IN_CONFIG_FILE 100
#define MAX_MATCHING_LINES_CAPACITY 150
#define MAX_STRING_LENGTH_CAPACITY 512
#define SINGLE_CONFIG_VALUE_SIZE 1
#define ZOOM_SCALE_FACTOR 0.15f
#define COLOR_CHANGE_FACTOR 16
#define SDL_DELAY_FACTOR 256
#define MIN_ZOOM_SCALE 0.5f
#define MAX_ZOOM_SCALE 5.0f

#ifdef DEBUG_MODE
    #define DEBUG_SHOW_LOC(fmt, ...) fprintf(stdout, "\n%s:%d:" CYN " %s():\n" RESET fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    #define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define DEBUG_SHOW_LOC(...) ((void)0)
    #define DEBUG_PRINTF(...) ((void)0)
#endif

void* check_ptr(void* ptr, char* message_on_failure __attribute__((unused)), const char* error_fetcher __attribute__((unused))) {
    if (ptr == NULL) {
        DEBUG_PRINTF("Something wrong in SDL. %s. %s\n", message_on_failure, error_fetcher);
        abort();
    }
    return ptr;
}

void check_code(int code, const char* error_fetcher __attribute__((unused))) {
    if (code < 0) {
        DEBUG_PRINTF("Something wrong in SDL. %s\n", error_fetcher);
        abort();
    }
}

void keyword_lines_into_array(char* file_path, char** destination_array, size_t* destination_array_index, size_t destination_array_max_capacity, char** keywords_source_array, size_t keywords_source_count) {
    void* file_content = check_ptr(SDL_LoadFile(file_path, NULL), "Couldn't find target file..", SDL_GetError());
    char* file_content_line = strtok((char*)file_content, "\n");

    // Search for the keywords in each line
    DEBUG_SHOW_LOC("Matching lines:\n");
    while (file_content_line != NULL && *destination_array_index < destination_array_max_capacity) {
        for (size_t i = 0; i < keywords_source_count; i++) {
            if (strstr(file_content_line, keywords_source_array[i])) {
                strncpy(destination_array[*destination_array_index], file_content_line, MAX_STRING_LENGTH_CAPACITY);
                (*destination_array_index)++;
                DEBUG_PRINTF("%s\n", file_content_line);
            }
        }
        file_content_line = strtok(NULL, "\n");
    }
    SDL_free(file_content);
}

void trim_keyword_prefix(char* text_line, char* keyword) {
    if (!text_line || !keyword)
        return;

    char* char_ptr = text_line;

    // For Org files
    if (*char_ptr == '*') {
        while (*char_ptr == '*') {
            char_ptr++;
        }
        char_ptr++; // Skip the extra space
    }

    size_t compare_length = strlen(keyword);
    if (strncmp(char_ptr, keyword, compare_length) == 0) {
        char_ptr += compare_length;
        char_ptr++; // TODO: Skip only if there is a space key here
    }

    memmove(text_line, char_ptr, strlen(char_ptr) + 1);
}

FILE* create_demo_conf_file(const char* conf_file_path) {
    FILE* file = check_ptr(fopen(conf_file_path, "wb"), "Error creating a file", SDL_GetError());

#ifdef _WIN32
    char generated_file_content[] = "file = \"C:\\Users\\USER\\todos.org\"\n"
                                    "keyword = \"TODO\"\n";
#else
    char generated_file_content[] = "file = \"/home/USER/todos.org\"\n"
                                    "keyword = \"TODO\"\n";
#endif
    size_t generated_file_content_size = sizeof(generated_file_content);

    fwrite(generated_file_content, sizeof(generated_file_content[0]), generated_file_content_size, file);

    fclose(file);
    return fopen(conf_file_path, "r");
}

int conf_file_lines_into_array(const char* file_path, char** file_lines_array) {
    // Get config file vars
    FILE* file = fopen(file_path, "r");
    if (!file) {
        DEBUG_SHOW_LOC("File '%s' not found. Creating demo file.\n", file_path);
        file = check_ptr(create_demo_conf_file(file_path), "Could not create the demo config file", SDL_GetError());
    }

    size_t curr_line = 0;

    char string_buffer[MAX_STRING_LENGTH_CAPACITY];

    // for every line from configuration file into buffer
    while (fgets(string_buffer, sizeof(string_buffer), file)) {

        // if length of buffer > 1 and buffer has a '\n', allocate a '\0' at the end
        size_t string_buffer_len = strlen(string_buffer);
        if (string_buffer_len > 0 && string_buffer[string_buffer_len - 1] == '\n') {
            string_buffer[string_buffer_len - 1] = '\0';
        }

        strncpy(file_lines_array[curr_line], string_buffer, MAX_STRING_LENGTH_CAPACITY);
        curr_line++;

        if (curr_line >= MAX_LINES_IN_CONFIG_FILE) {
            DEBUG_SHOW_LOC("Warning: Reached maximum number of lines in config file: %d max lines allowed.\n", MAX_LINES_IN_CONFIG_FILE);
            break;
        }
    }

    // Show the lines that are read
    DEBUG_SHOW_LOC("Lines read from '%s':\n", file_path);
    for (size_t i = 0; i < curr_line; i++) {
        DEBUG_PRINTF("%s:%zu: line read: %s\n", file_path, i + 1, file_lines_array[i]);
    }

    fclose(file);

    // return the number of lines
    return curr_line;
}

size_t extract_config_values(char* keyword, char** destination_array, size_t destination_array_length, char** source_array, size_t source_array_length) {

    size_t keyword_index = 0;
    size_t destination_array_index = 0;

    for (size_t i = 0; i < source_array_length; i++) {

        char* char_start_ptr = strchr(source_array[i], '"'); // first quote
        if (!char_start_ptr)
            continue;
        char_start_ptr++; // skip the quote

        char* char_end_ptr = strchr(char_start_ptr, '"'); // matching second quote
        if (!char_end_ptr)
            continue;

        char string_buffer[MAX_STRING_LENGTH_CAPACITY];
        size_t word_length = char_end_ptr - char_start_ptr;

        strncpy(string_buffer, char_start_ptr, word_length); // copy string from 'start' to 'end' into buf
        string_buffer[word_length] = '\0';

        // copy the substring within quotes into the destination array
        if (destination_array_index < destination_array_length) {
            if (strstr(source_array[i], keyword) != NULL) {
                strncpy(destination_array[destination_array_index], string_buffer, MAX_STRING_LENGTH_CAPACITY);
                keyword_index++;
                destination_array_index++;
            }
        }
    }
    return keyword_index;
}

int parse_single_user_value(char** user_value_array, size_t user_value_count, int default_value) {
    if (user_value_count < 1 || !isdigit(user_value_array[0][0])) {
        return default_value;
    } else {
        return atoi(user_value_array[0]);
    }
}

int centered_window_x_position(int user_screen_width, int window_width) {
    return (user_screen_width / 2) - (window_width / 2);
}

void initialize_string_array(char** array, size_t array_max_size, size_t max_strlen) {
    for (size_t i = 0; i < array_max_size; i++) {
        array[i] = (char*)malloc(max_strlen);
    }
}

void destroy_string_array(char** array, size_t array_max_size) {
    for (size_t i = 0; i < array_max_size; i++) {
        free(array[i]);
    }
}

int target_paths_modified(char** target_paths_array, size_t target_paths_count, time_t* last_mtime) {
    time_t latest_mtime = 0;
    for (size_t i = 0; i < target_paths_count; i++) {
        struct stat file_stat;
        check_code(stat(target_paths_array[i], &file_stat), "File modification check failed.");

        if (file_stat.st_mtime > latest_mtime) {
            latest_mtime = file_stat.st_mtime;
        }
    }

    int modified = (*last_mtime) != latest_mtime;
    if (modified) {
        *last_mtime = latest_mtime;
    }

    return modified;
}

float clamp(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}

int main(int argc __attribute__((unused)), char* argv[] __attribute__((unused))) {

    char* keywords_array[MAX_KEYWORDS];
    char* target_paths_array[MAX_TARGET_PATHS];
    char* conf_file_lines_array[MAX_LINES_IN_CONFIG_FILE];
    char* matching_lines_array[MAX_MATCHING_LINES_CAPACITY];
    char* window_height_array[SINGLE_CONFIG_VALUE_SIZE];
    char* window_width_array[SINGLE_CONFIG_VALUE_SIZE];
    char* window_x_position_array[SINGLE_CONFIG_VALUE_SIZE];
    char* window_y_position_array[SINGLE_CONFIG_VALUE_SIZE];
    char conf_file_path[MAX_STRING_LENGTH_CAPACITY];
    size_t keywords_count;
    size_t target_paths_count;
    size_t conf_file_line_count;
    size_t matching_lines_curr_line_index;
    size_t window_height_count;
    size_t window_width_count;
    size_t window_x_position_count;
    size_t window_y_position_count;
    SDL_Color bg_color = {24, 128, 64, 240};

    initialize_string_array(keywords_array, MAX_KEYWORDS, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(target_paths_array, MAX_TARGET_PATHS, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(conf_file_lines_array, MAX_LINES_IN_CONFIG_FILE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(matching_lines_array, MAX_MATCHING_LINES_CAPACITY, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_height_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_width_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_x_position_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_y_position_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);

    const char* window_title = "WhatWasiDoing";
#ifdef _WIN32
    const char* conf_file_filename = "\\.currTasks.conf";
#else
    const char* conf_file_filename = "/.currTasks.conf";
#endif

    // Get the user's home dir
    const char* user_env_home = getenv("HOME");
    if (!user_env_home) {
        perror("Error getting home directory");
        return -1;
    }
    // Get the full file path to the config file (join the strings)
    snprintf(conf_file_path, sizeof(conf_file_path), "%s%s", user_env_home, conf_file_filename);
    conf_file_line_count = conf_file_lines_into_array(conf_file_path, conf_file_lines_array);

    target_paths_count = extract_config_values("file", target_paths_array, MAX_TARGET_PATHS, conf_file_lines_array, conf_file_line_count);
    keywords_count = extract_config_values("keyword", keywords_array, MAX_KEYWORDS, conf_file_lines_array, conf_file_line_count);
    window_x_position_count = extract_config_values("initial_window_x", window_x_position_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    window_y_position_count = extract_config_values("initial_window_y", window_y_position_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    window_width_count = extract_config_values("initial_window_width", window_width_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    window_height_count = extract_config_values("initial_window_height", window_height_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);

    DEBUG_SHOW_LOC("Read target paths from config file\n");
    matching_lines_curr_line_index = 0;
    time_t last_mtime = 0;
    for (size_t i = 0; i < target_paths_count; i++) {
        DEBUG_PRINTF(YEL "%zu: %s " RESET "\n", i + 1, target_paths_array[i]);
        keyword_lines_into_array(target_paths_array[i], matching_lines_array, &matching_lines_curr_line_index, MAX_MATCHING_LINES_CAPACITY, keywords_array, keywords_count);

        struct stat file_stat;
        check_code(stat(target_paths_array[i], &file_stat), "File modification check failed.");
        if (file_stat.st_mtime > last_mtime) {
            last_mtime = file_stat.st_mtime;
        }
    }

    // SDL /////////////////////////////////////////////////////////
    DEBUG_SHOW_LOC("Initializing SDL_ttf\n");
    check_code(TTF_Init(), TTF_GetError());

    DEBUG_SHOW_LOC("Loading font\n");
#ifdef _WIN32
    const char* font_path = "C:\\Users\\Eyu\\Projects\\probe\\nerd-fonts\\patched-fonts\\Iosevka\\IosevkaNerdFont-Regular.ttf";
#else
    const char* font_path = "/usr/share/fonts/adobe-source-code-pro/SourceCodePro-Regular.otf";
#endif
    int font_size = 36;
    TTF_Font* font_ptr = check_ptr(TTF_OpenFont(font_path, font_size), "Error loading font", TTF_GetError());

    DEBUG_SHOW_LOC("Initializing SDL\n");
    check_code(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());

    int user_display_index = 0;
    SDL_DisplayMode user_display_mode_info;
    SDL_GetDesktopDisplayMode(user_display_index, &user_display_mode_info);

    const int default_window_width = user_display_mode_info.w - 60;
    const int default_window_height = 50;
    const int default_window_x_position = (user_display_mode_info.w / 2) - (default_window_width / 2); // center - half-width
    const int default_window_y_position = user_display_mode_info.h - default_window_height;

    int window_width = parse_single_user_value(window_width_array, window_width_count, default_window_width);
    int window_height = parse_single_user_value(window_height_array, window_height_count, default_window_height);                 // height downwards
    int window_position_x = parse_single_user_value(window_x_position_array, window_x_position_count, default_window_x_position); // left border position
    int window_position_y = parse_single_user_value(window_y_position_array, window_y_position_count, default_window_y_position); // top border position

    float zoom_scale = 1.0;

    // Adjust centering after receiving the user's desired width; TODO: make it the user's option
    window_position_x = centered_window_x_position(user_display_mode_info.w, window_width);

    Uint32 window_sdl_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
    SDL_Window* window_ptr = check_ptr(SDL_CreateWindow(window_title, window_position_x, window_position_y, window_width, window_height, window_sdl_flags), "Couldn't create a SDL window", SDL_GetError());
    SDL_bool window_is_bordered = SDL_FALSE;
    SDL_bool window_is_resizable = SDL_FALSE;
    SDL_bool window_is_on_top = SDL_TRUE;

    SDL_RendererFlags sdl_renderer_flags = SDL_RENDERER_SOFTWARE;
    SDL_Renderer* renderer_ptr = check_ptr(SDL_CreateRenderer(window_ptr, -1, sdl_renderer_flags), "Couldn't create an SDL renderer", SDL_GetError());

    DEBUG_SHOW_LOC("Entering SDL Event Loop\n");
    bool window_should_run = true;
    while (window_should_run) {
        SDL_Event sdl_events;
        while (SDL_PollEvent(&sdl_events)) {
            switch (sdl_events.type) {
            case SDL_QUIT: {
                window_should_run = false;
                break;
            }
            case SDL_WINDOWEVENT: {
                if (sdl_events.window.event == SDL_WINDOWEVENT_CLOSE) {
                    window_should_run = false;
                }
                break;
            }
            case SDL_KEYDOWN: {
                if (sdl_events.key.keysym.sym == SDLK_r && (sdl_events.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.r -= 16;
                } else if (sdl_events.key.keysym.sym == SDLK_g && (sdl_events.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.g -= 16;
                } else if (sdl_events.key.keysym.sym == SDLK_b && (sdl_events.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.b -= 16;
                } else if (sdl_events.key.keysym.sym == SDLK_a && (sdl_events.key.keysym.mod & KMOD_SHIFT)) {
                    bg_color.a -= 16;
                } else if (sdl_events.key.keysym.sym == SDLK_EQUALS && (sdl_events.key.keysym.mod & KMOD_CTRL)) {
                    zoom_scale = clamp(zoom_scale + (ZOOM_SCALE_FACTOR * zoom_scale), MIN_ZOOM_SCALE, MAX_ZOOM_SCALE);
                } else if (sdl_events.key.keysym.sym == SDLK_MINUS && (sdl_events.key.keysym.mod & KMOD_CTRL)) {
                    zoom_scale = clamp(zoom_scale - (ZOOM_SCALE_FACTOR * zoom_scale), MIN_ZOOM_SCALE, MAX_ZOOM_SCALE);
                } else if (sdl_events.key.keysym.sym == SDLK_0 && (sdl_events.key.keysym.mod & KMOD_CTRL)) {
                    zoom_scale = 1.0;
                } else {
                    switch (sdl_events.key.keysym.sym) {
                    case SDLK_r: {
                        bg_color.r += 16;
                        break;
                    }
                    case SDLK_g: {
                        bg_color.g += 16;
                        break;
                    }
                    case SDLK_b: {
                        bg_color.b += 16;
                        break;
                    }
                    case SDLK_a: {
                        bg_color.a += 16;
                        break;
                    }
                    case SDLK_t: {
                        // store current size and position
                        int window_current_width, window_current_height, window_current_x_position, window_current_y_position;
                        SDL_GetWindowSize(window_ptr, &window_current_width, &window_current_height);
                        SDL_GetWindowPosition(window_ptr, &window_current_x_position, &window_current_y_position);

                        if (window_is_bordered == SDL_FALSE) {
                            window_is_bordered = SDL_TRUE;
                        } else {
                            window_is_bordered = SDL_FALSE;
                        }
                        if (window_is_resizable == SDL_FALSE) {
                            window_is_resizable = SDL_TRUE;
                        } else {
                            window_is_resizable = SDL_FALSE;
                        }

                        SDL_SetWindowBordered(window_ptr, window_is_bordered);
                        SDL_SetWindowResizable(window_ptr, window_is_resizable);
                        // restore window from stored size and position to compensate for wierd SDL behavior
                        SDL_SetWindowSize(window_ptr, window_current_width, window_current_height);
                        SDL_SetWindowPosition(window_ptr, window_current_x_position, window_current_y_position);
                        break;
                    }
                    case SDLK_z: {
                        SDL_SetWindowPosition(window_ptr, window_position_x, window_position_y);
                        SDL_SetWindowSize(window_ptr, window_width, window_height);
                        break;
                    }
                    case SDLK_p: {
                        if (window_is_on_top == SDL_FALSE) {
                            window_is_on_top = SDL_TRUE;
                        } else {
                            window_is_on_top = SDL_FALSE;
                        }
                        SDL_SetWindowAlwaysOnTop(window_ptr, window_is_on_top);
                        break;
                    }
                    }
                }
            }
            }
        }

        SDL_SetRenderDrawColor(renderer_ptr, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        DEBUG_SHOW_LOC("BG Colors:\n"
                       "\tr = %u\n"
                       "\tg = %u\n"
                       "\tb = %u\n"
                       "\ta = %u\n",
                       bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderClear(renderer_ptr);

        int y_offset = 0;

        if (matching_lines_curr_line_index == 0) {
            const char* text_as_none = "NONE";
            SDL_Color text_color = {255, 255, 255, 255};
            SDL_Surface* text_surface = TTF_RenderText_Blended(font_ptr, text_as_none, text_color);
            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer_ptr, text_surface);
            SDL_Rect src_rect = {0, 0, text_surface->w, text_surface->h};
            SDL_Rect dst_rect = {(window_width / 2) - (int)(window_width / 2.5), y_offset, text_surface->w * zoom_scale, text_surface->h * zoom_scale};
            SDL_RenderCopy(renderer_ptr, text_texture, &src_rect, &dst_rect);
            SDL_FreeSurface(text_surface);
            SDL_DestroyTexture(text_texture);
        }
        for (size_t i = 0; i < matching_lines_curr_line_index; i++) {

            SDL_Color text_color = {255, 255, 255, 255};
            SDL_Surface* text_surface;

            if (i == 0) {
                char matching_lines_first_line_text[MAX_STRING_LENGTH_CAPACITY];
                const char* matching_lines_first_line_prefix = "Current Task: ";

                // trim out the keywords
                for (size_t i = 0; i < keywords_count; i++) {
                    trim_keyword_prefix(matching_lines_array[0], keywords_array[i]);
                }
                snprintf(matching_lines_first_line_text, sizeof(matching_lines_first_line_text), "%s%s", matching_lines_first_line_prefix, matching_lines_array[i]);
                text_surface = check_ptr(TTF_RenderText_Blended(font_ptr, matching_lines_first_line_text, text_color), "Error loading a font text surface", TTF_GetError());
            } else {
                text_surface = check_ptr(TTF_RenderText_Blended(font_ptr, matching_lines_array[i], text_color), "Error loading a font text surface", TTF_GetError());
            }

            SDL_Texture* text_texture = check_ptr(SDL_CreateTextureFromSurface(renderer_ptr, text_surface), "Couldn't create a SDL texture", TTF_GetError());
            SDL_Rect src_rect = {0, 0, text_surface->w, text_surface->h};
            SDL_Rect dst_rect = {0, y_offset, text_surface->w * zoom_scale, text_surface->h * zoom_scale};
            SDL_RenderCopy(renderer_ptr, text_texture, &src_rect, &dst_rect);

            y_offset += text_surface->h * zoom_scale;

            SDL_FreeSurface(text_surface);
            SDL_DestroyTexture(text_texture);
        }

        SDL_RenderPresent(renderer_ptr);
        SDL_Delay(SDL_DELAY_FACTOR);

        if (target_paths_modified(target_paths_array, target_paths_count, &last_mtime) != 0) {
            DEBUG_SHOW_LOC("Read target paths from config file\n");
            matching_lines_curr_line_index = 0;
            for (size_t i = 0; i < target_paths_count; i++) {
                DEBUG_PRINTF(YEL "%zu: %s" RESET "\n", i + 1, target_paths_array[i]);
                keyword_lines_into_array(target_paths_array[i], matching_lines_array, &matching_lines_curr_line_index, MAX_MATCHING_LINES_CAPACITY, keywords_array, keywords_count);
            }
        }
    }

    DEBUG_SHOW_LOC("Destroying Renderer\n");
    SDL_DestroyRenderer(renderer_ptr);

    DEBUG_SHOW_LOC("Destroying Window\n");
    SDL_DestroyWindow(window_ptr);

    DEBUG_SHOW_LOC("Closing SDL_ttf\n");
    TTF_CloseFont(font_ptr);
    TTF_Quit();

    DEBUG_SHOW_LOC("Quitting SDL\n");
    SDL_Quit();

    destroy_string_array(conf_file_lines_array, MAX_LINES_IN_CONFIG_FILE);
    destroy_string_array(matching_lines_array, MAX_LINES_IN_CONFIG_FILE);
    destroy_string_array(target_paths_array, MAX_TARGET_PATHS);
    destroy_string_array(keywords_array, MAX_KEYWORDS);
    destroy_string_array(window_height_array, SINGLE_CONFIG_VALUE_SIZE);
    destroy_string_array(window_width_array, SINGLE_CONFIG_VALUE_SIZE);
    destroy_string_array(window_x_position_array, SINGLE_CONFIG_VALUE_SIZE);
    destroy_string_array(window_y_position_array, SINGLE_CONFIG_VALUE_SIZE);

    DEBUG_SHOW_LOC("Exiting Application\n");

    return 0;
}
