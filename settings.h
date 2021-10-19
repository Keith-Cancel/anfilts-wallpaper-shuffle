#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

bool    config_check_directory();

void    config_save_enabled(bool enabled);
void    config_save_path(const wchar_t* path);
void    config_save_period_seconds(unsigned period);

bool     config_get_enabled();
size_t   config_get_path_length();
void     config_get_path(wchar_t* path, size_t capacity);
unsigned config_get_period_seconds();

#endif