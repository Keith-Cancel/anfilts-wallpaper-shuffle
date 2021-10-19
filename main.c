#define _WIN32_WINNT 0x0600
#define _WIN32_IE    0x0600

#include <stdio.h>
#include <windows.h>

#include "tray-dialogs.h"
#include "timed-thread.h"
#include "settings.h"
#include "utils.h"

/* A bunch of call back functions which update the saved settings
** and currently running thread.
*/
bool enable_startup(void* param) {
    wchar_t path[MAX_PATH + 1] = { 0 };
    DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
    if(len == 0 || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        return false;
    }

    HKEY key = NULL;
    if(RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &key) != ERROR_SUCCESS) {
        return false;
    }
    LSTATUS ret = RegSetValueExW(key, L"Anfilt's Wallpaper Shuffle", 0, REG_SZ, (BYTE*)path, (len+1) * sizeof(wchar_t));
    RegCloseKey(key);
    return ret == ERROR_SUCCESS;
}

bool disable_startup(void* param) {
    HKEY key = NULL;
    if(RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &key) != ERROR_SUCCESS) {
        return false;
    }
    LSTATUS ret = RegDeleteValueW(key, L"Anfilt's Wallpaper Shuffle");
    RegCloseKey(key);
    return ret == ERROR_SUCCESS;
}

bool is_startup_on(void* param) {
    HKEY key = NULL;
    if(RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &key) != ERROR_SUCCESS) {
        return false;
    }
    LSTATUS ret = RegQueryValueExW(key, L"Anfilt's Wallpaper Shuffle", NULL, NULL, NULL, NULL);
    RegCloseKey(key);
    return ret == ERROR_SUCCESS;
}

void set_enable(void* param, bool enable) {
    timer_thread_set_enable(param, enable);
    config_save_enabled(enable);
}

void set_freq_sec(void* param, int freq) {
    timer_thread_set_period(param, freq);
    config_save_period_seconds(freq);
}

size_t get_path(void* param, wchar_t* dest, size_t buffer_sz) {
    ThreadFlexBytes* dir = timer_thread_get_search_dir(param);
    size_t bytes = thread_flex_bytes_copy_to_fixed_buffer(dir, dest, buffer_sz * sizeof(wchar_t));
    // convert bytes to characters
    return (bytes / sizeof(wchar_t)) - 1;
}

size_t set_path(void* param, const wchar_t* path) {
    ThreadFlexBytes* dir = timer_thread_get_search_dir(param);
    size_t len   = wcslen(path);
    size_t bytes = (len + 1) * sizeof(wchar_t);
    thread_flex_bytes_copy_from(dir, path, bytes);
    config_save_path(path);
    return len;
}

void initialize_thread_path_from_settings(TimerThread* thread) {
    ThreadFlexBytes* dir = timer_thread_get_search_dir(thread);
    size_t len = config_get_path_length();
    wchar_t path[len + 1];
    config_get_path(path, len + 1);
    thread_flex_bytes_copy_from(dir, path, sizeof(wchar_t) * (len + 1));
}

int main(int argc, char* argv[]) {
    // Use a named mutex to prevent multiple instances from running
    HANDLE mutex_handle = CreateMutex(NULL, TRUE, "anfilts_wallpaper_shuffler");
    if(ERROR_ALREADY_EXISTS == GetLastError()) {
        MessageBox(NULL, "Anfilt's Wallpaper Shuffler is already running!", "Error!", MB_OK | MB_ICONERROR);
        return -1;
    }
    if(!config_check_directory()) {
        MessageBox(NULL, "Making settings directory failed, settings may not be saved!", "Error!", MB_OK | MB_ICONWARNING);
    }
    // Initialize the timing thread and create the windows for the context menu
    int          return_val = 0;
    TimerThread* thread     = timer_thread_create(config_get_period_seconds());

    HWND         window     = NULL;
    UpdateCallBacks funcs   = {
        .user_param      = thread,
        .get_freq_sec    = (unsigned (*)(void *))timer_thread_get_period,
        .set_freq_sec    = set_freq_sec,
        .get_path        = get_path,
        .set_path        = set_path,
        .run_now         = (void (*)(void *))timer_thread_run_now,
        .set_enable      = set_enable,
        .is_enabled      = (bool (*)(void *))timer_thread_is_enabled,
        .disable_startup = disable_startup,
        .enable_startup  = enable_startup,
        .is_startup_on   = is_startup_on
    };

    if(thread == NULL) {
        MessageBox(NULL, "Failed to create timing thread!", "Error!", MB_OK | MB_ICONERROR);
        goto clean_up_mutex;
    }

    // Setup the initial values for the thread
    timer_thread_set_enable(thread, config_get_enabled());
    initialize_thread_path_from_settings(thread);

    window = create_tray_window(funcs);
    if(window == NULL) {
        MessageBox(NULL, "Failed to create tray window!", "Error!", MB_OK | MB_ICONERROR);
        return_val = -1;
        goto clean_up_thread;
    }

    if(!init_tray(window)) {
        MessageBox(NULL, "Failed to create tray icon!", "Error!", MB_OK | MB_ICONERROR);
        return_val = -1;
        goto clean_tray;
    }

    timer_thread_start(thread);

    MSG msg = { 0 };
    // Windows Event Loop
    while(GetMessage(&msg, NULL, 0,  0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    clean_tray:
        destroy_tray_window(window);
    clean_up_thread:
        timer_thread_close(thread);
    clean_up_mutex:
        ReleaseMutex(mutex_handle);
        CloseHandle(mutex_handle);
    return return_val;
}