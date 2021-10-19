
// Version define constants docs, need to be defined before includes
// https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?view=msvc-160
#define WINVER        0x0602
#define _WIN32_WINNT  0x0602
#define NTDDI_VERSION 0x06020000

#include <windows.h>
#include <initguid.h>
#include <shobjidl.h>

#include "wallpaper-obj.h"

struct wallpaper_obj_s {
    IDesktopWallpaper* wall;
    LPMALLOC           mem;

};

WallpaperObj* wallpaper_obj_create() {
    WallpaperObj* obj = malloc(sizeof(WallpaperObj));
    if(obj == NULL) {
        return NULL;
    }
    HRESULT res = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    // S_FALSE means it was already intialized for this thread which is fine
    // https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex#remarks
    if(res != S_OK && res != S_FALSE) {
        free(obj);
        return NULL;
    }
    res = CoGetMalloc(1, &(obj->mem));
    if(res != S_OK) {
        free(obj);
        return NULL;
    }

    res = CoCreateInstance(
        &CLSID_DesktopWallpaper,
        0,
        CLSCTX_LOCAL_SERVER,
        &IID_IDesktopWallpaper,
        (void**)&(obj->wall)
    );
    if(res != S_OK) {
        obj->mem->lpVtbl->Release(obj->mem);
        return NULL;
    }
    return obj;
}

unsigned wallpaper_obj_monitor_count(WallpaperObj* obj) {
    UINT count  = 0;
    HRESULT res = obj->wall->lpVtbl->GetMonitorDevicePathCount(obj->wall, &count);
    if(res != S_OK) {
        // just return 1 for now since I hope most machines have 1 monitor
        // although I guess headless servers are thing
        return 1;
    }
    return count;
}

void wallpaper_obj_set_wallpaper(WallpaperObj* obj, unsigned monitor_index, const wchar_t* image_path) {
    wchar_t* monitor_path = NULL;
    HRESULT res = obj->wall->lpVtbl->GetMonitorDevicePathAt(obj->wall, monitor_index, &monitor_path);
    // I guess just bail for now
    if(res != S_OK) {
        // I have been unabled to get the result to fail on my machine, so I am unsure
        // if it still will allocate memory if it fails so just check in case for now
        if(monitor_path != NULL) {
            obj->mem->lpVtbl->Free(obj->mem, monitor_path);
        }
        return;
    }
    // if this fails I am just gonna bail anyways, for now so no need to check the HRESULT
    // I still need to figure out how I want to log errors for this application.
    obj->wall->lpVtbl->SetWallpaper(obj->wall, monitor_path, image_path);
    // free the monitor path allocated by GetMonitorDevicePathAt
    obj->mem->lpVtbl->Free(obj->mem, monitor_path);
}

void wallpaper_obj_release(WallpaperObj* obj) {
    obj->wall->lpVtbl->Release(obj->wall);
    obj->mem->lpVtbl->Release(obj->mem);
    free(obj);
    CoUninitialize();
}