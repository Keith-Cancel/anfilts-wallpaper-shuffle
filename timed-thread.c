#include <windows.h>

#include <time.h>
#include <stdint.h>
#include <stdatomic.h>

#include "timed-thread.h"
#include "wchar-list.h"
#include "wallpaper-obj.h"
#include "utils.h"


#if ATOMIC_BOOL_LOCK_FREE != 2
    #error "atomic_bool is not lock free on this system"
#endif

#if ATOMIC_INT_LOCK_FREE != 2
    #error "atomic_int is not lock free on this system"
#endif

#if ATOMIC_LONG_LOCK_FREE != 2
    #error "atomic_long is not lock free on this system"
#endif

_Static_assert(sizeof(atomic_uint)  >= 4, "atomic_uint is tool small!");
_Static_assert(sizeof(atomic_ullong) >= 8, "atomic_ullong is tool small!");

struct timer_thread_s {
    HANDLE           thrd;
    HANDLE           timer;
    atomic_uintptr_t search_dir;
    atomic_ullong    period_sec;
    atomic_uint      last_run;
    atomic_bool      enabled;
    atomic_bool      run_once;
    atomic_bool      stop;
};

static const LARGE_INTEGER hundred_ns = {
    .QuadPart = -1
};

static LARGE_INTEGER seconds_to_large_int(uint32_t value) {
    LARGE_INTEGER large = { 0 };
    large.QuadPart = value;
    // convert seconds to 100 ns time intervals, and negative for relative time
    large.QuadPart *= -10 * 1000 * 1000;
    return large;
}

void set_wallpaper(TimerThread* data, WallpaperObj* wall) {
    ThreadFlexBytes* dir = timer_thread_get_search_dir(data);
    size_t   bytes       = 0;
    wchar_t* path        = thread_flex_bytes_allocate_local_copy(dir, &bytes, sizeof(wchar_t) * 2);
    if(dir == NULL) {
        return;
    }
    size_t len    = (bytes / sizeof(wchar_t)) + 2 - 1; // 2 extra chars - null char
    path[len - 2] = L'\\';
    path[len - 1] = L'*';
    path[len]     = L'\0';
    bytes        += sizeof(wchar_t) * 2;

    wcharList        file_list = { 0 };
    WIN32_FIND_DATAW file_data = { 0 };

    wchar_list_init(&file_list);


    // iterate through every file in the directory
    HANDLE find = FindFirstFileW(path, &file_data);
    if(find == INVALID_HANDLE_VALUE) {
        free(path);
        return;
    }

    do {
        if((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        // png, bmp, jpg, gif, ect...
        if(!has_img_ext(file_data.cFileName)) {
            continue;
        }
        wchar_list_add_node(&file_list, file_data.cFileName);

    } while (FindNextFileW(find, &file_data));

    FindClose(find);

    len--; // we don't want the char '*'
    for(unsigned i = 0; i < wallpaper_obj_monitor_count(wall); i++) {
        wcharNode* random = wchar_list_remove_random_node(&file_list);
        if(random == NULL) {
            break;
        }
        size_t    str_len = wcslen(random->name);
        size_t    needs   = sizeof(wchar_t) * (str_len + len + 1);
        if(bytes < needs) {
            void* tmp = realloc(path, needs);
            if(tmp == NULL) {
                MessageBox(NULL, "Memory allocation failed! Quiting Wallpaper Swapper.", "Error!", MB_OK | MB_ICONERROR);
                ExitProcess(0);
            }
            path = tmp;
        }
        memcpy(&(path[len]), random->name, str_len * sizeof(wchar_t));
        path[len + str_len] = L'\0';
        wchar_node_free(random);
        wallpaper_obj_set_wallpaper(wall, i, path);
    }
    wchar_list_free_all(&file_list);
    free(path);
}

DWORD WINAPI timer_thread(LPVOID param) {
    TimerThread* data  = (TimerThread*)param;
    WallpaperObj* wall = wallpaper_obj_create();
    if(wall == NULL) {
        MessageBox(NULL, "Can not create Wallpaper Object! Quiting Wallpaper Swapper.", "Error!", MB_OK | MB_ICONERROR);
        ExitProcess(0);
    }

    data->last_run = time(NULL);
    for(;;) {
        if(data->stop) {
            break;
        }
        LARGE_INTEGER time_period = seconds_to_large_int(data->period_sec);
        if(!SetWaitableTimer(data->timer, &time_period, 0, NULL, NULL, 0)) {
            MessageBox(NULL, "Thread Timer Failed! Quiting Wallpaper Swapper.", "Error!", MB_OK | MB_ICONERROR);
            ExitProcess(0);
            // Maybe could retry with something like this
            // Sleep(10);
            // continue;
        }
        if(WaitForSingleObject(data->timer, INFINITE) != WAIT_OBJECT_0) {
            // just try again I guess
            continue;
        }
        if(!data->enabled && !data->run_once) {
            continue;
        }
        set_wallpaper(data, wall);
        data->last_run  = time(NULL);
        data->run_once  = false;
    }
    wallpaper_obj_release(wall);
    return 0;
}

TimerThread* timer_thread_create(unsigned period_sec) {
    TimerThread* data = malloc(sizeof(TimerThread));
    if(data == NULL) {
        return NULL;
    }

    data->enabled    = true;
    data->stop       = false;
    data->run_once   = false;
    data->period_sec = period_sec;
    data->search_dir = (uintptr_t)thread_flex_bytes_create(sizeof(wchar_t) * (MAX_PATH + 1));
    if(data->search_dir == (uintptr_t)NULL) {
        goto free_data;
    }

    data->thrd = CreateThread(
        NULL,                                                 // default security attributes
        128 * 4096,                                           // set stack size 512K
        timer_thread,
        data,                                                 // timer thread param
        STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, // creation flags
        NULL                                                  // Do not need the thread ID
    );

    if(data->thrd == NULL) {
        goto free_search_dir;
    }

    data->timer = CreateWaitableTimer(NULL, TRUE, NULL);
    if(data->timer == NULL) {
        goto free_thread;
    }
    return data;
    // Clean up code for failures
free_thread:
    CloseHandle(data->thrd);
free_search_dir:
    thread_flex_bytes_free((ThreadFlexBytes*)data->search_dir);
free_data:
    free(data);
    return NULL;
}

void timer_thread_start(TimerThread* data) {
    data->last_run = time(NULL);
    ResumeThread(data->thrd);
}

bool timer_thread_set_period(TimerThread* data, unsigned period_sec) {
    LARGE_INTEGER new_time = hundred_ns;
    unsigned waited_for = time(NULL) - data->last_run;
    if(period_sec > waited_for) {
        // Account for previously amount of time spent waiting
        new_time = seconds_to_large_int(period_sec - waited_for);
    }
    data->period_sec = period_sec;
    return SetWaitableTimer(data->timer, &new_time, 0, NULL, NULL, 0) == TRUE;
}

unsigned timer_thread_get_period(TimerThread* data) {
    return data->period_sec;
}

void timer_thread_run_now(TimerThread* data) {
    data->run_once = true;
    SetWaitableTimer(data->timer, &hundred_ns, 0, NULL, NULL, 0);
}

void timer_thread_set_enable(TimerThread* data, bool enabled) {
    data->enabled = enabled;
    // maybe run now if last run is greate than period, dunno
}

bool timer_thread_is_enabled(TimerThread* data) {
    bool enabled = data->enabled;
    return enabled;
}

void timer_thread_stop(TimerThread* data) {
    data->stop = true;
    timer_thread_run_now(data);
}

void timer_thread_close(TimerThread* data) {
    timer_thread_stop(data);
    WaitForSingleObject(data->thrd, INFINITE);
    CloseHandle(data->thrd);
    CloseHandle(data->timer);
    thread_flex_bytes_free((ThreadFlexBytes*)data->search_dir);
    // free the thread data struct
    free(data);
}

ThreadFlexBytes* timer_thread_get_search_dir(TimerThread* data) {
    ThreadFlexBytes* tmp = (ThreadFlexBytes*)data->search_dir;
    return tmp;
}