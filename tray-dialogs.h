#ifndef TRAY_DIA_H
#define TRAY_DIA_H

#include <windows.h>
#include <stdbool.h>

typedef struct update_funcs_s {
    // User paremeter passed to each of the callbacks
    void*    user_param;
    // Callbacks for the time to run in seconds
    void     (*set_freq_sec)(void*, int);
    unsigned (*get_freq_sec)(void*);
    // Path Callbacks
    size_t   (*set_path)(void*, const wchar_t*);
    size_t   (*get_path)(void*, wchar_t*, size_t);
    // Callback to for enable on startup
    bool     (*disable_startup)(void*);
    bool     (*enable_startup) (void*);
    bool     (*is_startup_on)(void*);
    // Callback to control timed update
    void     (*run_now)(void*);
    void     (*set_enable)(void*, bool);
    bool     (*is_enabled)(void*);

} UpdateCallBacks;

HWND create_tray_window(UpdateCallBacks funcs);
bool init_tray(HWND hWnd);
void destroy_tray_window(HWND hWnd);


#endif // TRAY_DIA_H