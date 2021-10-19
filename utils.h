#ifndef WALLPAPER_UTILS_H
#define WALLPAPER_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define PATH_EXIST      1
#define PATH_ERROR      0
#define PATH_NO_EXIST  -1
#define PATH_NO_PERM   -2
#define PATH_NOT_DIR   -3

int dir_exists(const wchar_t* path);

int dir_select_dialog(wchar_t* start_path, wchar_t* path, size_t length);

uint64_t rand64();
uint64_t rand64_less_than(uint64_t value);

bool     has_img_ext(const wchar_t* name);
#endif