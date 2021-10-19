#include <windows.h>
#include "settings.h"
#include "utils.h"

#include <stdio.h>

#define APP  L"WallPaperShuffle"

static const wchar_t default_path[]     = L"%HOMEDRIVE%%HOMEPATH%\\Pictures";
static const wchar_t config_directory[] = L"%APPDATA%\\anfilt";
static const wchar_t user_file[]        = L"%APPDATA%\\anfilt\\wallpaper-shuffle.ini";
static const wchar_t local_file[]       = L".\\wallpaper-shuffle.ini";

static const wchar_t* get_file_path(wchar_t* buffer, size_t capacity) {
    if(buffer == NULL || capacity == 0) {
        return local_file;
    }
    size_t len = ExpandEnvironmentStringsW(user_file, buffer, capacity);
    if(len == 0) {
        MessageBox(NULL, "Unable use AppData path for settings! Using current directory instead!", "Error!", MB_OK | MB_ICONERROR);
        return local_file;
    }
    return buffer;
}

bool config_check_directory() {
    wchar_t buffer[MAX_PATH + 1];
    if(ExpandEnvironmentStringsW(config_directory, buffer, MAX_PATH) == 0) {
        return false;
    }
    BOOL ret = CreateDirectoryW(buffer, NULL);
    if(ret == 0) {
        DWORD err = GetLastError();
        if(err == ERROR_ALREADY_EXISTS) {
            ret = 1;
        }
    }
    return ret != 0;
}

// No write private profile int function exists for some reason
BOOL WritePrivateIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, UINT lpInt, LPCWSTR lpFileName) {
    UINT tmp     = lpInt;
    unsigned cnt = 1;
    // get number of characters
    while(tmp >= 10) {
        tmp /= 10;
        cnt++;
    }
    wchar_t int_str[cnt + 1];
    // Convert the unsigend int to a str so I can save it
    for(unsigned i = 0; i < cnt; i++) {
        tmp = lpInt / 10;
        int_str[cnt - 1 - i] = L'0' + (lpInt - (tmp * 10));
        lpInt = tmp;
    }
    int_str[cnt] = L'\0';
    return WritePrivateProfileStringW(lpAppName, lpKeyName, int_str, lpFileName);
}

void config_save_enabled(bool enabled) {
    wchar_t buffer[MAX_PATH + 1];
    WritePrivateIntW(APP, L"Enabled", enabled, get_file_path(buffer, MAX_PATH));
}

void config_save_path(const wchar_t* path) {
    wchar_t buffer[MAX_PATH + 1];
    WritePrivateIntW(APP, L"SearchPathLength", wcslen(path), get_file_path(buffer, MAX_PATH));
    WritePrivateProfileStringW(APP, L"SearchPath", path, get_file_path(buffer, MAX_PATH));
}

void config_save_period_seconds(unsigned period) {
    wchar_t buffer[MAX_PATH + 1];
    WritePrivateIntW(APP, L"PeriodSec", period, get_file_path(buffer, MAX_PATH));
}

bool config_get_enabled() {
    wchar_t buffer[MAX_PATH + 1];
    return GetPrivateProfileIntW(APP, L"Enabled", 1, get_file_path(buffer, MAX_PATH)) == true;
}

unsigned config_get_period_seconds() {
    wchar_t buffer[MAX_PATH + 1];
    return GetPrivateProfileIntW(APP, L"PeriodSec", 60*60, get_file_path(buffer, MAX_PATH));
}

size_t config_get_path_length() {
    wchar_t buffer[MAX_PATH + 1];
    return GetPrivateProfileIntW(APP, L"SearchPathLength", (sizeof(default_path)/sizeof(wchar_t)) - 1, get_file_path(buffer, MAX_PATH));
}

void config_get_path(wchar_t* path, size_t capacity) {
    wchar_t buffer[MAX_PATH + 1];
    GetPrivateProfileStringW(APP, L"SearchPath", default_path, path, capacity, get_file_path(buffer, MAX_PATH));
}