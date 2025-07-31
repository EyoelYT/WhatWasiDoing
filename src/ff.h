// clang-format Language: C
#ifndef FF_H_
#define FF_H_

#define FF_PATH_MAX 512

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FF_StringArray FF_StringArray;
void ffStringArrayInit(FF_StringArray* strct, size_t initialCapacity);
void ffStringArrayAppend(FF_StringArray* strct, const char* str);
void ffStringArrayDestroy(FF_StringArray* strct);
void ffStringArrayPrintList(FF_StringArray* strct);
static int ffIsFontFile(const char* file_name);
static void ffScanDir(const char* path, FF_StringArray* out);
void ffGetPlatformFontDirs(FF_StringArray* dirs);
int ffFindFonts(const FF_StringArray* dirs, FF_StringArray* out);

// Implementation:

typedef struct FF_StringArray {
    char** items;
    size_t size;
    size_t capacity;
} FF_StringArray;

void ffStringArrayInit(FF_StringArray* strct, size_t initialCapacity) {
    strct->items = malloc(sizeof(char*) * initialCapacity);
    strct->size = 0;
    strct->capacity = strct->items ? initialCapacity : 0;
}

void ffStringArrayAppend(FF_StringArray* strct, const char* str) {
    if (strct->capacity < strct->size + 1) {
        size_t newCap = strct->capacity ? strct->capacity * 2 : 4;
        char** tmp = realloc(strct->items, newCap * sizeof(char*));
        if (!tmp)
            return; // TODO: we may want to signal error
        strct->items = tmp;
        strct->capacity = newCap;
    }
    strct->items[strct->size] = strdup(str);
    strct->size++;
}

void ffStringArrayDestroy(FF_StringArray* strct) {
    if (!strct->items)
        return;
    for (size_t i = 0; i < strct->size; i++)
        free(strct->items[i]);
    free(strct->items);
    strct->items = NULL;
    strct->size = strct->capacity = 0;
}

void ffStringArrayPrintList(FF_StringArray* strct) {
    if (!strct->items) {
        printf("no items");
        return;
    }
    for (size_t i = 0; i < strct->size; i++) {
        printf("Font: %s\n", strct->items[i]);
    }
}

#ifdef __WIN32
    #include <Shlobj.h>
    #include <windows.h>

static int ffIsFontFile(const char* file_name) {
    const char* ext = strchr(file_name, '.');
    if (!ext)
        return 0;
    ext++;
    if (_stricmp(ext, "ttf") == 0)
        return 1;
    if (_stricmp(ext, "otf") == 0)
        return 1;
    if (_stricmp(ext, "ttc") == 0)
        return 1;
    if (_stricmp(ext, "woff") == 0)
        return 1;
    if (_stricmp(ext, "woff2") == 0)
        return 1;
    return 0;
}

static void ffScanDir(const char* path, FF_StringArray* out) {
    char search_path[FF_PATH_MAX];
    WIN32_FIND_DATAA file_data;
    HANDLE file_handle = INVALID_HANDLE_VALUE;

    snprintf(search_path, FF_PATH_MAX, "%s\\*.*", path);
    file_handle = FindFirstFileA(search_path, &file_data);

    if (file_handle == INVALID_HANDLE_VALUE)
        return; // TODO: return error here!

    do {
        if (strcmp(file_data.cFileName, ".") == 0 || strcmp(file_data.cFileName, "..") == 0)
            continue;

        char some_full_path[FF_PATH_MAX];
        snprintf(some_full_path, FF_PATH_MAX, "%s\\%s", path, file_data.cFileName);

        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ffScanDir(some_full_path, out);
        } else if (ffIsFontFile(file_data.cFileName)) {
            ffStringArrayAppend(out, some_full_path);
        }
    } while (FindNextFileA(file_handle, &file_data));

    FindClose(file_handle);
}

void ffGetPlatformFontDirs(FF_StringArray* dirs) {
    PWSTR fonts_dir = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Fonts, 0, NULL, &fonts_dir))) {
        char multi_byte_str[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, fonts_dir, -1, multi_byte_str, MAX_PATH, NULL, NULL);
        ffStringArrayAppend(dirs, multi_byte_str);
        CoTaskMemFree(fonts_dir);
    } else {
        ffStringArrayAppend(dirs, "C:\\Windows\\Fonts"); // The default that I know of
    }
    const char* local = getenv("LOCALAPPDATA");
    if (local) {
        char local_font_dir[MAX_PATH];
        snprintf(local_font_dir, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts", local);
        ffStringArrayAppend(dirs, local_font_dir);
    }
}

#else
    #include <dirent.h>
    #include <sys/stat.h>

static int ffIsFontFile(const char* file_name) {
    const char* ext = strchr(file_name, '.');
    if (!ext)
        return 0;
    ext++;
    if (strcasecmp(ext, "ttf") == 0)
        return 1;
    if (strcasecmp(ext, "otf") == 0)
        return 1;
    if (strcasecmp(ext, "ttc") == 0)
        return 1;
    if (strcasecmp(ext, "woff") == 0)
        return 1;
    if (strcasecmp(ext, "woff2") == 0)
        return 1;
    return 0;
}

static void ffScanDir(const char* path, FF_StringArray* out) {
    DIR* dir_stream = opendir(path);
    if (!dir_stream)
        return; // TODO: return error here!

    struct dirent* dir_entry;
    struct stat file_st;
    char some_full_path[FF_PATH_MAX];

    while ((dir_entry = readdir(dir_stream))) {
        if ((strcmp(dir_entry->d_name, ".") == 0) || strcmp(dir_entry->d_name, "..") == 0)
            continue;

        size_t dir_path_len = strlen(path);
        if (path[dir_path_len - 1] == '/')
            snprintf(some_full_path, FF_PATH_MAX, "%s%s", path, dir_entry->d_name);
        else
            snprintf(some_full_path, FF_PATH_MAX, "%s/%s", path, dir_entry->d_name);

        if (stat(some_full_path, &file_st) != 0)
            continue; // if no file-sys, skip

        if (S_ISDIR(file_st.st_mode)) {
            ffScanDir(some_full_path, out);
        } else if ((S_ISREG(file_st.st_mode)) && (ffIsFontFile(dir_entry->d_name))) {
            ffStringArrayAppend(out, some_full_path);
        }
    }

    closedir(dir_stream);
}

void ffGetPlatformFontDirs(FF_StringArray* dirs) {
    ffStringArrayAppend(dirs, "/usr/share/fonts");
    ffStringArrayAppend(dirs, "/usr/local/share/fonts");
    const char* home = getenv("HOME");
    if (home) {
        char font_dir[FF_PATH_MAX];
        if (snprintf(font_dir, FF_PATH_MAX, "%s/.local/share/fonts", home) < FF_PATH_MAX)
            ffStringArrayAppend(dirs, font_dir);
        if (snprintf(font_dir, FF_PATH_MAX, "%s/.fonts", home) < FF_PATH_MAX)
            ffStringArrayAppend(dirs, font_dir);
    }
    #ifdef __APPLE__ // not tested on apple platforms!!
    ffStringArrayAppend(dirs, "/Library/Fonts");
    ffStringArrayAppend(dirs, "/System/Library/Fonts");
    if (home) {
        char font_dir[FF_PATH_MAX];
        if (snprintf(font_dir, FF_PATH_MAX, "%s/Library/fonts", home) < FF_PATH_MAX)
            ffStringArrayAppend(dirs, font_dir);
    }
    #endif
}

#endif

int ffFindFonts(const FF_StringArray* dirs, FF_StringArray* out) {
    for (size_t i = 0; i < dirs->size; i++)
        ffScanDir(dirs->items[i], out);
    return 0;
}

#endif // FF_H_
