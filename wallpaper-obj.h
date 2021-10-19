#ifndef DESKTOP_WALLPAPER_OBJ_H
#define DESKTOP_WALLPAPER_OBJ_H

struct wallpaper_obj_s;
typedef struct wallpaper_obj_s WallpaperObj;

WallpaperObj* wallpaper_obj_create();
void          wallpaper_obj_release(WallpaperObj* obj);
unsigned      wallpaper_obj_monitor_count(WallpaperObj* obj);
void          wallpaper_obj_set_wallpaper(WallpaperObj* obj, unsigned monitor_index, const wchar_t* image_path);

#endif