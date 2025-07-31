#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_messagebox.h>
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
#include <time.h>

#define CYN "\x1B[36m"
#define YEL "\x1B[33m"
#define RESET "\x1B[0m"

// do not tweak
#define SINGLE_CONFIG_VALUE_SIZE 1

// tweakables
#define MAX_TARGET_PATHS 90
#define MAX_KEYWORDS 10
#define MAX_LINES_IN_CONFIG_FILE 100
#define MAX_MATCHING_LINES_CAPACITY 150
#define MAX_STRING_LENGTH_CAPACITY 512
#define COLOR_CHANGE_FACTOR 16
#define SDL_DELAY_FACTOR 256
#define ZOOM_SCALE_FACTOR 0.15f
#define MIN_ZOOM_SCALE 0.5f
#define MAX_ZOOM_SCALE 5.0f
#define DEMO_TARGET_FILE "todos.org"
#define DEMO_EXAMPLE_KEYWORD "TODO"
#define CONFIG_FILE_NAME ".currTasks.conf"

#ifdef DEBUG_MODE
    #define DEBUG_SHOW_LOC(fmt, ...) fprintf(stdout, "\n%s:%d:" CYN " %s():\n" RESET fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    #define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define DEBUG_SHOW_LOC(...) ((void)0)
    #define DEBUG_PRINTF(...) ((void)0)
#endif

void* check_ptr(void* ptr, const char* message_on_failure __attribute__((unused)), const char* error_fetcher __attribute__((unused))) {
    if (ptr == NULL) {
        DEBUG_PRINTF("Something (ptr) wrong in SDL. %s. %s\n", message_on_failure, error_fetcher);
        abort();
    }
    return ptr;
}

void check_code(int code, const char* error_fetcher __attribute__((unused))) {
    if (code < 0) {
        DEBUG_PRINTF("Something (code) wrong in SDL. %s\n", error_fetcher);
        abort();
    }
}

bool file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

void keyword_lines_into_array(const char* file_path, char** destination_array, size_t* destination_array_index, size_t destination_array_max_capacity, char** keywords_source_array, size_t keywords_source_count) {
    if (!file_exists(file_path)) {
        DEBUG_SHOW_LOC("SKIPPING file %s since it doesn't exist.\n", file_path);

        char message[MAX_STRING_LENGTH_CAPACITY];
        snprintf(message, MAX_STRING_LENGTH_CAPACITY, "Skipping file %s since it doesn't exist.", file_path);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, NULL);
        return;
    }
    void* file_content = check_ptr(SDL_LoadFile(file_path, NULL), "Couldn't find target file..", SDL_GetError());
    char* file_content_line = strtok((char*)file_content, "\n");

    // Search for the keywords in each line
    DEBUG_SHOW_LOC("Matching lines:\n");
    while (file_content_line != NULL && *destination_array_index < destination_array_max_capacity) {
        for (size_t i = 0; i < keywords_source_count; i++) {
            if (strstr(file_content_line, keywords_source_array[i])) {
                snprintf(destination_array[*destination_array_index], MAX_STRING_LENGTH_CAPACITY, "%s", file_content_line);
                (*destination_array_index)++;
                DEBUG_PRINTF("%s\n", file_content_line);
            }
        }
        file_content_line = strtok(NULL, "\n");
    }
    SDL_free(file_content);
}

void trim_leading_item_prefix(char* text_line) {
    if (!text_line)
        return;

    char* char_ptr = text_line;

    // Skip leading whitespace first
    while (*char_ptr == ' ' || *char_ptr == '\t') {
        char_ptr++;
    }

    if (*char_ptr == '*') { // * Org headings
        while (*char_ptr == '*') {
            char_ptr++;
        }
        if (*char_ptr == ' ') {
            char_ptr++; // Skip the space if it is there
        }
    } else if (*char_ptr == '-') { // - Items
        while (*char_ptr == '-') {
            char_ptr++;
        }
        if (*char_ptr == ' ') {
            char_ptr++;
        }
    } else if (*char_ptr == '+') { // + Items
        while (*char_ptr == '+') {
            char_ptr++;
        }
        if (*char_ptr == ' ') {
            char_ptr++;
        }
    } else if (*char_ptr == '#') { // # Items
        while (*char_ptr == '#') {
            char_ptr++;
        }
        if (*char_ptr == ' ') {
            char_ptr++;
        }
    }

    memmove(text_line, char_ptr, strlen(char_ptr) + 1);
}

void trim_keyword_prefix(char* text_line, char* keyword) {
    if (!text_line || !keyword || strcmp(keyword, "") == 0)
        return;

    char* char_ptr = text_line;

    trim_leading_item_prefix(char_ptr);

    size_t compare_length = strlen(keyword);
    if (strncmp(char_ptr, keyword, compare_length) == 0) {
        char_ptr += compare_length;
        if (*char_ptr == ' ') {
            char_ptr++; // Skip only if there is a space key here
        }
    }

    memmove(text_line, char_ptr, strlen(char_ptr) + 1);
}

void send_ok_cancel_message_box(const char* title, const char* message, const char* message_on_failure) {
    SDL_MessageBoxButtonData buttons[2];

    // FIXME: Keyboard shortcuts don't work right in WSL environment
    buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    buttons[0].buttonid = 0;
    buttons[0].text = "OK";

    buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    buttons[1].buttonid = 1;
    buttons[1].text = "CANCEL";

    SDL_MessageBoxData message_box_data;
    message_box_data.flags = SDL_MESSAGEBOX_INFORMATION;
    message_box_data.window = NULL;
    message_box_data.title = title;
    message_box_data.message = message;
    message_box_data.numbuttons = 2;
    message_box_data.buttons = buttons;
    message_box_data.colorScheme = NULL;

    int hit_button;
    SDL_ShowMessageBox(&message_box_data, &hit_button);
    if (hit_button != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Exiting", message_on_failure, NULL);
        abort();
    }
}

void create_demo_target_file(const char* todo_file_path) {
    if (file_exists(todo_file_path)) {
        return;
    }
    send_ok_cancel_message_box("Confirm", "Create a demo \"todos.org\" file in $HOME?", "Aborting since there is no 'file' set in config");

    FILE* file = check_ptr(fopen(todo_file_path, "wb"), "Error creating a file\n", SDL_GetError());

    char file_content[MAX_STRING_LENGTH_CAPACITY];
    snprintf(file_content, MAX_STRING_LENGTH_CAPACITY, "* TODO Checkout the demo file located at %s\n", todo_file_path);
    fwrite(file_content, sizeof(char), strlen(file_content), file);
    fclose(file);
}

FILE* create_demo_conf_file(const char* conf_file_path) {
    FILE* file = check_ptr(fopen(conf_file_path, "wb"), "Error creating a file", SDL_GetError());

    const char* user_env_home = check_ptr(getenv("HOME"), "$HOME environment variable not set", SDL_GetError());
    const char* demo_target_file = DEMO_TARGET_FILE;
    const char* demo_example_keyword = DEMO_EXAMPLE_KEYWORD;

    char f_content_0[MAX_STRING_LENGTH_CAPACITY];
    char f_content_1[MAX_STRING_LENGTH_CAPACITY];
    char todo_file_string[MAX_STRING_LENGTH_CAPACITY];
#ifdef _WIN32
    snprintf(f_content_0, MAX_STRING_LENGTH_CAPACITY, "file = \"%s\\%s\"\n", user_env_home, demo_target_file);
    snprintf(f_content_1, MAX_STRING_LENGTH_CAPACITY, "keyword = \"%s\"\n", demo_example_keyword);
    snprintf(todo_file_string, MAX_STRING_LENGTH_CAPACITY, "\\%s", demo_target_file);
#else
    snprintf(f_content_0, MAX_STRING_LENGTH_CAPACITY, "file = \"%s/%s\"\n", user_env_home, demo_target_file);
    snprintf(f_content_1, MAX_STRING_LENGTH_CAPACITY, "keyword = \"%s\"\n", demo_example_keyword);
    snprintf(todo_file_string, MAX_STRING_LENGTH_CAPACITY, "/%s", demo_target_file);
#endif

    size_t generated_file_content_size = strlen(f_content_0) + strlen(f_content_1) + 1; //  +1 to add space for '\0'

    char generated_file_content[generated_file_content_size];
    snprintf(generated_file_content, generated_file_content_size, "%s%s", f_content_0, f_content_1);

    fwrite(generated_file_content, sizeof(char), strlen(generated_file_content), file);
    fclose(file);

    char todo_file_path[MAX_STRING_LENGTH_CAPACITY];
    snprintf(todo_file_path, strlen(todo_file_string) + strlen(user_env_home) + 1, "%s%s", user_env_home, todo_file_string); // +1 to add space for '\0'
    create_demo_target_file(todo_file_path);
    return fopen(conf_file_path, "r");
}

int conf_file_lines_into_array(const char* file_path, char** file_lines_array, const char* conf_file_filename) {

    FILE* file = fopen(file_path, "r");

    if (!file) {
        char message[MAX_STRING_LENGTH_CAPACITY];
        snprintf(message, MAX_STRING_LENGTH_CAPACITY, "Create a \"%s\" config file in $HOME?", conf_file_filename);

        DEBUG_SHOW_LOC("File '%s' not found. Trying to create a demo file.\n", file_path);
        // Get config file vars
        send_ok_cancel_message_box("Confirm", message, "Aborting since there is no config file.");
        file = check_ptr(create_demo_conf_file(file_path), "Could not create the demo config file", SDL_GetError());
    }

    size_t curr_line = 0;

    if (file) {
        char string_buffer[MAX_STRING_LENGTH_CAPACITY];

        // for every line from configuration file into buffer
        while (fgets(string_buffer, sizeof(string_buffer), file)) {

            // if length of buffer > 1 and buffer has a '\n', allocate a '\0' at the end
            size_t string_buffer_len = strlen(string_buffer);
            if (string_buffer_len > 0 && string_buffer[string_buffer_len - 1] == '\n') {
                string_buffer[string_buffer_len - 1] = '\0';
            }

            snprintf(file_lines_array[curr_line], MAX_STRING_LENGTH_CAPACITY, "%s", string_buffer);
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
    }

    // return the number of lines found
    return curr_line;
}

bool array_contains_string(char** array, size_t array_size, char* string) {
    for (size_t i = 0; i < array_size; i++) {
        if (strstr(array[i], string)) {
            return true;
        }
    }
    return false;
}

bool has_leading_nonblank_char(char** character_set, size_t character_set_len, char* curr_line) {

    for (size_t i = 0; i < character_set_len; i++) {

        char* character = character_set[i];
        char* line = curr_line;

        while (line && *line != '\0') {
            if (strncmp(line, " ", 1) == 0) { // if space, skip char
                line++;
                continue;
            }

            if (strncmp(line, character, strlen(character)) == 0) {
                return true;
            } else { // if not comment char, skip char
                line++;
                continue;
            }
        }
    }
    return false;
}

size_t extract_config_values(char* keyword, char** destination_array, size_t destination_array_length, char** source_array, size_t source_array_length) {

    size_t destination_array_index = 0;

    for (size_t i = 0; i < source_array_length; i++) {

        // copy the substring within quotes if it is to the right the keyword (into the destination array)
        if (destination_array_index < destination_array_length && strstr(source_array[i], keyword) != NULL) { // and keyword in source array

            char* comment_chars[] = {"#", ";"}; // change comment characters here!
            if (has_leading_nonblank_char(comment_chars, sizeof(comment_chars) / sizeof(comment_chars[0]), source_array[i])) {
                continue;
            }

            char* char_start_ptr = strchr(source_array[i], '"'); // first quote
            if (!char_start_ptr)
                continue;
            char_start_ptr++; // skip the quote

            char* char_end_ptr = strchr(char_start_ptr, '"'); // matching second quote
            if (!char_end_ptr)
                continue;

            char keyword_value_string[MAX_STRING_LENGTH_CAPACITY];
            size_t word_length = char_end_ptr - char_start_ptr;
            snprintf(keyword_value_string, word_length + 1, "%s", char_start_ptr);

            if (!array_contains_string(destination_array, destination_array_index, keyword_value_string)) { // avoid duplicates
                snprintf(destination_array[destination_array_index], MAX_STRING_LENGTH_CAPACITY, "%s", keyword_value_string);
            }

            destination_array_index++;
        }
    }

    return destination_array_index;
}

int parse_single_user_value_int(char** user_value_array, size_t user_value_count, int default_value) {
    if (user_value_count < 1 || !isdigit(user_value_array[0][0])) {
        return default_value;
    }
    return atoi(user_value_array[0]);
}

int parse_single_user_value_bool(char** user_value_array, size_t user_value_count, bool default_value) {
    if (user_value_count < 1) {
        return default_value;
    }
    if (strcmp(user_value_array[0], "true") == 0) {
        return true;
    }
    return false;
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

int target_paths_modified(char** target_paths_array, size_t target_paths_count, time_t* last_mtime, bool* target_path_nonexistence_array) {
    // track the latest modification time from all of the file paths
    time_t latest_mtime = 0;

    for (size_t i = 0; i < target_paths_count; i++) {

        // Logic:
        //
        // !file exists && previously exist
        //     -> update latest "modified time"
        //     -> update to track file as not existing
        // file exists && previously not exist
        //     -> uptate latest "modified time"
        //     -> update to track file as existing
        // !file exists && previously not exist
        //     -> continue as is
        // file exists && previously exist
        //     -> check for file modification
        //     -> continue as is

        if (!file_exists(target_paths_array[i]) && !target_path_nonexistence_array[i]) {
            target_path_nonexistence_array[i] = true;
            latest_mtime = time(NULL); // current time
            break;
        }

        if (file_exists(target_paths_array[i]) && target_path_nonexistence_array[i]) {
            target_path_nonexistence_array[i] = false;
            latest_mtime = time(NULL); // current time
            break;
        }

        if (!file_exists(target_paths_array[i]) && target_path_nonexistence_array[i]) {
            continue;
        }

        struct stat file_stat;
        check_code(stat(target_paths_array[i], &file_stat), "File modification check failed.");

        if (file_stat.st_mtime > latest_mtime) {
            latest_mtime = file_stat.st_mtime;
        }
    }

    int modified = (*last_mtime) < latest_mtime;
    if (modified) {
        *last_mtime = latest_mtime;
    }

    return modified;
}

int path_modified(char* file_path, time_t* last_mtime, bool* conf_file_existence, size_t* conf_file_line_count) {
    // track the latest modification time from all of the file paths
    time_t latest_mtime = 0;
    // if first time file unexisted
    if (!file_exists(file_path) && *conf_file_existence) {
        time_t now = time(NULL); // current time
        latest_mtime = now;
        *conf_file_existence = false;
        *conf_file_line_count = 0;
    } else if (!file_exists(file_path) && !*conf_file_existence) { // if previously nonexisted, don't affect the latest modified time
        *conf_file_line_count = 0;
        latest_mtime = *last_mtime;
    } else if (file_exists(file_path)) { // if the file exists
        *conf_file_existence = true;
        struct stat file_stat;
        check_code(stat(file_path, &file_stat), "File modification check failed.");

        if (file_stat.st_mtime > latest_mtime) { // this is not getting true when config file is created??
            latest_mtime = file_stat.st_mtime;
        }
    }

    int modified = (*last_mtime) < latest_mtime;
    if (modified) {
        *last_mtime = latest_mtime;
    }

    return modified;
}

void render_text_line(SDL_Renderer* renderer_ptr, SDL_Surface* text_surface, int* y_offset, float zoom_scale) {
    SDL_Texture* text_texture = check_ptr(SDL_CreateTextureFromSurface(renderer_ptr, text_surface), "Couldn't create a SDL texture", TTF_GetError());
    SDL_Rect src_rect = {0, 0, text_surface->w, text_surface->h};
    SDL_Rect dst_rect = {0, *y_offset, text_surface->w * zoom_scale, text_surface->h * zoom_scale};
    SDL_RenderCopy(renderer_ptr, text_texture, &src_rect, &dst_rect);

    *y_offset += text_surface->h * zoom_scale;

    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_texture);
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

size_t calculate_user_entry_offset(size_t default_value, int entry_offset, size_t entry_count) {
    // (sum % chice_count) to make the result cycle around
    long idx = (long)default_value + (long)entry_offset;
    long n = (long)entry_count;

    // bring the value into [0 .. n-1]
    idx %= n;
    if (idx < 0)
        idx += n; // make positive

    return (size_t)idx;
}

void interpret_sdl_events(SDL_Window* window_ptr, SDL_bool* window_is_resizable, SDL_bool* window_is_bordered, SDL_bool* window_is_on_top,
                          bool* window_should_render, bool* window_should_run, int* window_position_x, int* window_position_y,
                          int* window_width, int* window_height, float* zoom_scale, int* user_entry_offset,
                          bool* config_file_should_be_read, SDL_Color* bg_color) {
    SDL_Event sdl_events;
    while (SDL_PollEvent(&sdl_events)) {
        switch (sdl_events.type) {
            case SDL_QUIT: {
                *window_should_run = false;
                break;
            }
            case SDL_WINDOWEVENT: {
                *window_should_render = true;
                if (sdl_events.window.event == SDL_WINDOWEVENT_CLOSE) {
                    *window_should_run = false;
                }
                break;
            }
            case SDL_KEYDOWN: {
                *window_should_render = true;
                if (sdl_events.key.keysym.mod & KMOD_SHIFT) {
                    switch (sdl_events.key.keysym.sym) {
                        case SDLK_r: {
                            bg_color->r -= COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_g: {
                            bg_color->g -= COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_b: {
                            bg_color->b -= COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_a: {
                            bg_color->a -= COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_UP: {
                            *user_entry_offset -= 1;
                            break;
                        }
                        case SDLK_DOWN: {
                            *user_entry_offset += 1;
                            break;
                        }
                    }
                } else if (sdl_events.key.keysym.mod & KMOD_CTRL) {
                    switch (sdl_events.key.keysym.sym) {
                        case SDLK_EQUALS: {
                            *zoom_scale = clamp(*zoom_scale + (ZOOM_SCALE_FACTOR * (*zoom_scale)), MIN_ZOOM_SCALE, MAX_ZOOM_SCALE);
                            break;
                        }
                        case SDLK_MINUS: {
                            *zoom_scale = clamp(*zoom_scale - (ZOOM_SCALE_FACTOR * (*zoom_scale)), MIN_ZOOM_SCALE, MAX_ZOOM_SCALE);
                            break;
                        }
                        case SDLK_0: {
                            *zoom_scale = 1.0;
                            break;
                        }
                    }
                } else {
                    switch (sdl_events.key.keysym.sym) {
                        case SDLK_0: {
                            *user_entry_offset = 0;
                            break;
                        }
                        case SDLK_c: {
                            *config_file_should_be_read = true;
                            break;
                        }
                        case SDLK_r: {
                            bg_color->r += COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_g: {
                            bg_color->g += COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_b: {
                            bg_color->b += COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_a: {
                            bg_color->a += COLOR_CHANGE_FACTOR;
                            break;
                        }
                        case SDLK_t: {
                            // store current size and position
                            int window_current_width, window_current_height, window_current_x_position, window_current_y_position;
                            SDL_GetWindowSize(window_ptr, &window_current_width, &window_current_height);
                            SDL_GetWindowPosition(window_ptr, &window_current_x_position, &window_current_y_position);

                            if (*window_is_bordered == SDL_FALSE) {
                                *window_is_bordered = SDL_TRUE;
                            } else {
                                *window_is_bordered = SDL_FALSE;
                            }
                            if (*window_is_resizable == SDL_FALSE) {
                                *window_is_resizable = SDL_TRUE;
                            } else {
                                *window_is_resizable = SDL_FALSE;
                            }

                            SDL_SetWindowBordered(window_ptr, *window_is_bordered);
                            SDL_SetWindowResizable(window_ptr, *window_is_resizable);
                            // restore window from stored size and position to compensate for wierd SDL behavior
                            SDL_SetWindowSize(window_ptr, window_current_width, window_current_height);
                            SDL_SetWindowPosition(window_ptr, window_current_x_position, window_current_y_position);
                            break;
                        }
                        case SDLK_z: {
                            SDL_SetWindowPosition(window_ptr, *window_position_x, *window_position_y);
                            SDL_SetWindowSize(window_ptr, *window_width, *window_height);
                            break;
                        }
                        case SDLK_p: {
                            if (*window_is_on_top == SDL_FALSE) {
                                *window_is_on_top = SDL_TRUE;
                            } else {
                                *window_is_on_top = SDL_FALSE;
                            }
                            SDL_SetWindowAlwaysOnTop(window_ptr, *window_is_on_top);
                            break;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc __attribute__((unused)), char* argv[] __attribute__((unused))) {

    char* keywords_array[MAX_KEYWORDS];
    char* target_paths_array[MAX_TARGET_PATHS];
    bool target_path_nonexistence_array[MAX_TARGET_PATHS];
    char* conf_file_lines_array[MAX_LINES_IN_CONFIG_FILE];
    char* matching_lines_array[MAX_MATCHING_LINES_CAPACITY];
    char* window_height_array[SINGLE_CONFIG_VALUE_SIZE];
    char* window_width_array[SINGLE_CONFIG_VALUE_SIZE];
    char* window_x_position_array[SINGLE_CONFIG_VALUE_SIZE];
    char* window_y_position_array[SINGLE_CONFIG_VALUE_SIZE];
    char* first_entry_only_array[SINGLE_CONFIG_VALUE_SIZE];
    char* trim_out_keywords_array[SINGLE_CONFIG_VALUE_SIZE];
    char conf_file_path[MAX_STRING_LENGTH_CAPACITY];
    size_t keywords_count;
    size_t target_paths_count;
    size_t conf_file_line_count;
    size_t matching_lines_curr_line_index;
    size_t window_height_count;
    size_t window_width_count;
    size_t window_x_position_count;
    size_t window_y_position_count;
    size_t first_entry_only_count;
    size_t trim_out_keywords_count;
    SDL_Color bg_color = {24, 128, 64, 240};

    initialize_string_array(keywords_array, MAX_KEYWORDS, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(target_paths_array, MAX_TARGET_PATHS, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(conf_file_lines_array, MAX_LINES_IN_CONFIG_FILE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(matching_lines_array, MAX_MATCHING_LINES_CAPACITY, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_height_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_width_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_x_position_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(window_y_position_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(first_entry_only_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);
    initialize_string_array(trim_out_keywords_array, SINGLE_CONFIG_VALUE_SIZE, MAX_STRING_LENGTH_CAPACITY);

    const char* window_title = "WhatWasiDoing";
    const char* conf_file_filename = CONFIG_FILE_NAME;

    // Get the user's home dir
    const char* user_env_home = check_ptr(getenv("HOME"), "Error getting $HOME envvar.", SDL_GetError());

    // Get the full file path to the config file (join the strings)
#ifdef _WIN32
    snprintf(conf_file_path, sizeof(conf_file_path), "%s\\%s", user_env_home, conf_file_filename);
#else
    snprintf(conf_file_path, sizeof(conf_file_path), "%s/%s", user_env_home, conf_file_filename);
#endif

    conf_file_line_count = conf_file_lines_into_array(conf_file_path, conf_file_lines_array, conf_file_filename);

    target_paths_count = extract_config_values("file", target_paths_array, MAX_TARGET_PATHS, conf_file_lines_array, conf_file_line_count);
    keywords_count = extract_config_values("keyword", keywords_array, MAX_KEYWORDS, conf_file_lines_array, conf_file_line_count);
    first_entry_only_count = extract_config_values("first_entry_only", first_entry_only_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    trim_out_keywords_count = extract_config_values("trim_out_keywords", trim_out_keywords_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);

    window_x_position_count = extract_config_values("initial_window_x", window_x_position_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    window_y_position_count = extract_config_values("initial_window_y", window_y_position_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    window_width_count = extract_config_values("initial_window_width", window_width_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
    window_height_count = extract_config_values("initial_window_height", window_height_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);

    matching_lines_curr_line_index = 0;
    bool conf_file_existence = true;
    time_t conf_file_last_mtime = 0;
    {
        struct stat file_stat;
        check_code(stat(conf_file_path, &file_stat), "File modification check failed.");
        if (file_stat.st_mtime > conf_file_last_mtime) {
            conf_file_last_mtime = file_stat.st_mtime;
        }
    }
    time_t target_paths_last_mtime = 0;
    // use target_paths_modified to update "the existence array" and "the last modification time"
    if (target_paths_modified(target_paths_array, target_paths_count, &target_paths_last_mtime, target_path_nonexistence_array) != 0) {
        DEBUG_SHOW_LOC("Read target paths from config file, and keyword lines from the target paths.\n");
        for (size_t i = 0; i < target_paths_count; i++) {
            DEBUG_PRINTF(YEL "%zu: %s " RESET "\n", i + 1, target_paths_array[i]);
            keyword_lines_into_array(target_paths_array[i], matching_lines_array, &matching_lines_curr_line_index, MAX_MATCHING_LINES_CAPACITY, keywords_array, keywords_count);

            if (i > 0) { // check if there are files in the first place
                struct stat target_path_file_stat;
                check_code(stat(target_paths_array[i], &target_path_file_stat), "File modification check failed.");
                if (target_path_file_stat.st_mtime > target_paths_last_mtime) {
                    target_paths_last_mtime = target_path_file_stat.st_mtime;
                }
            }
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
    const bool default_show_first_entry_only = true;
    const bool default_trim_out_keywords = false;

    int window_width = parse_single_user_value_int(window_width_array, window_width_count, default_window_width);
    int window_height = parse_single_user_value_int(window_height_array, window_height_count, default_window_height);                 // height downwards
    int window_position_x = parse_single_user_value_int(window_x_position_array, window_x_position_count, default_window_x_position); // left border position
    int window_position_y = parse_single_user_value_int(window_y_position_array, window_y_position_count, default_window_y_position); // top border position
    bool first_entry_only_setting = parse_single_user_value_bool(first_entry_only_array, first_entry_only_count, default_show_first_entry_only);
    bool trim_out_keywords_setting = parse_single_user_value_bool(trim_out_keywords_array, trim_out_keywords_count, default_trim_out_keywords);

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
    int user_entry_offset = 0;
    bool window_should_run = true;
    bool window_should_render = false;
    bool config_file_should_be_read = false;
    while (window_should_run) {
        interpret_sdl_events(window_ptr, &window_is_resizable, &window_is_bordered, &window_is_on_top, &window_should_render,
                             &window_should_run, &window_position_x, &window_position_y, &window_width, &window_height,
                             &zoom_scale, &user_entry_offset, &config_file_should_be_read, &bg_color);

        if (window_should_render) {
            SDL_SetRenderDrawColor(renderer_ptr, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            DEBUG_SHOW_LOC("BG Colors:\n"
                           "\tr = %u\n"
                           "\tg = %u\n"
                           "\tb = %u\n"
                           "\ta = %u\n",
                           bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderClear(renderer_ptr);

            int y_offset = 0;

            // when no entries are found, show "NONE"
            if (matching_lines_curr_line_index == 0) {
                const char* text_as_none = "NONE";
                SDL_Color text_color = {255, 255, 255, 255};
                SDL_Surface* text_surface = TTF_RenderText_Blended(font_ptr, text_as_none, text_color);

                render_text_line(renderer_ptr, text_surface, &y_offset, zoom_scale);
            } else if (first_entry_only_setting) {
                // show only first entry
                for (size_t i = 0; i < 1; i++) {

                    SDL_Color text_color = {255, 255, 255, 255};
                    SDL_Surface* text_surface;
                    size_t matching_lines_array_user_adjusted_idx = calculate_user_entry_offset(i, user_entry_offset, matching_lines_curr_line_index);

                    if (i == 0) {
                        char matching_lines_first_line_text[MAX_STRING_LENGTH_CAPACITY];
                        const char* matching_lines_first_line_prefix = "Current Task: ";

                        // trim out item prefixes and keywords
                        for (size_t j = 0; j < keywords_count; j++) {
                            trim_keyword_prefix(matching_lines_array[matching_lines_array_user_adjusted_idx], keywords_array[j]);
                        }
                        snprintf(matching_lines_first_line_text, sizeof(matching_lines_first_line_text), "%s%s", matching_lines_first_line_prefix, matching_lines_array[matching_lines_array_user_adjusted_idx]);
                        text_surface = check_ptr(TTF_RenderText_Blended(font_ptr, matching_lines_first_line_text, text_color), "Error loading a font text surface", TTF_GetError());
                    } else {
                        text_surface = check_ptr(TTF_RenderText_Blended(font_ptr, matching_lines_array[matching_lines_array_user_adjusted_idx], text_color), "Error loading a font text surface", TTF_GetError());
                    }

                    render_text_line(renderer_ptr, text_surface, &y_offset, zoom_scale);
                }
            } else { // iterate over all of them
                for (size_t i = 0; i < matching_lines_curr_line_index; i++) {

                    SDL_Color text_color = {255, 255, 255, 255};
                    SDL_Surface* text_surface;
                    size_t matching_lines_array_user_adjusted_idx = calculate_user_entry_offset(i, user_entry_offset, matching_lines_curr_line_index);

                    if (trim_out_keywords_setting) {
                        // trim out item prefixes and keywords in the current line
                        for (size_t j = 0; j < keywords_count; j++) {
                            trim_keyword_prefix(matching_lines_array[matching_lines_array_user_adjusted_idx], keywords_array[j]);
                        }
                    }

                    if (i == 0) { // first shown entry should have an identifier prefix
                        const char* matching_lines_first_line_prefix = "Current Task: ";
                        char matching_lines_first_line_text[MAX_STRING_LENGTH_CAPACITY];
                        snprintf(matching_lines_first_line_text, sizeof(matching_lines_first_line_text), "%s%s", matching_lines_first_line_prefix, matching_lines_array[matching_lines_array_user_adjusted_idx]);
                        text_surface = check_ptr(TTF_RenderText_Blended(font_ptr, matching_lines_first_line_text, text_color), "Error loading a font text surface", TTF_GetError());
                    } else {
                        text_surface = check_ptr(TTF_RenderText_Blended(font_ptr, matching_lines_array[matching_lines_array_user_adjusted_idx], text_color), "Error loading a font text surface", TTF_GetError());
                    }

                    render_text_line(renderer_ptr, text_surface, &y_offset, zoom_scale);
                }
            }

            SDL_RenderPresent(renderer_ptr);
            window_should_render = false;
        }
        SDL_Delay(SDL_DELAY_FACTOR);

        if (path_modified(conf_file_path, &conf_file_last_mtime, &conf_file_existence, &conf_file_line_count) != 0 || config_file_should_be_read) {
            window_should_render = true;
            config_file_should_be_read = false;
            if (conf_file_existence) {
                conf_file_line_count = conf_file_lines_into_array(conf_file_path, conf_file_lines_array, conf_file_filename);
                target_paths_count = extract_config_values("file", target_paths_array, MAX_TARGET_PATHS, conf_file_lines_array, conf_file_line_count);
                keywords_count = extract_config_values("keyword", keywords_array, MAX_KEYWORDS, conf_file_lines_array, conf_file_line_count);
                first_entry_only_count = extract_config_values("first_entry_only", first_entry_only_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
                first_entry_only_setting = parse_single_user_value_bool(first_entry_only_array, first_entry_only_count, default_show_first_entry_only);
                trim_out_keywords_count = extract_config_values("trim_out_keywords", trim_out_keywords_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
                trim_out_keywords_setting = parse_single_user_value_bool(trim_out_keywords_array, trim_out_keywords_count, default_trim_out_keywords);
            } else {
                conf_file_line_count = 0;
                target_paths_count = extract_config_values("file", target_paths_array, MAX_TARGET_PATHS, conf_file_lines_array, conf_file_line_count);
                keywords_count = extract_config_values("keyword", keywords_array, MAX_KEYWORDS, conf_file_lines_array, conf_file_line_count);
                first_entry_only_count = extract_config_values("first_entry_only", first_entry_only_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
                first_entry_only_setting = parse_single_user_value_bool(first_entry_only_array, first_entry_only_count, default_show_first_entry_only);
                trim_out_keywords_count = extract_config_values("trim_out_keywords", trim_out_keywords_array, SINGLE_CONFIG_VALUE_SIZE, conf_file_lines_array, conf_file_line_count);
                trim_out_keywords_setting = parse_single_user_value_bool(trim_out_keywords_array, trim_out_keywords_count, default_trim_out_keywords);
            }

            DEBUG_SHOW_LOC("Read target paths from config file\n");
            matching_lines_curr_line_index = 0;
            for (size_t i = 0; i < target_paths_count; i++) {
                DEBUG_PRINTF(YEL "%zu: %s" RESET "\n", i + 1, target_paths_array[i]);
                keyword_lines_into_array(target_paths_array[i], matching_lines_array, &matching_lines_curr_line_index, MAX_MATCHING_LINES_CAPACITY, keywords_array, keywords_count);
            }
        } else if (target_paths_modified(target_paths_array, target_paths_count, &target_paths_last_mtime, target_path_nonexistence_array) != 0) {
            window_should_render = true;
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
