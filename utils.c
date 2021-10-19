#include <stdbool.h>

#include <windows.h>
#include <shlobj.h>
#include <immintrin.h>

#include "utils.h"

int dir_exists(const wchar_t* dpath) {
    // fastest method seems to be use GetFileAttributesEx
    // https://stackoverflow.com/questions/8991192/check-the-file-size-without-opening-file-in-c
    WIN32_FIND_DATAA info = { 0 };
    BOOL ret = GetFileAttributesExW(
        dpath,
        GetFileExInfoStandard,
        &info
    );
    if(ret == 0) {
        int error = GetLastError();
        if(error == 2) {
            return PATH_NO_EXIST;
        }
        if(error == 5) {
            return PATH_NO_PERM;
        }
        return PATH_ERROR;
    }
    if(!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return PATH_NOT_DIR;
    }
    return PATH_EXIST;
}

static INT CALLBACK dir_select_callback(HWND hWnd, UINT msg, LPARAM lParam, LPARAM lpData) {
    switch(msg) {
        case BFFM_INITIALIZED:
            if((wchar_t*)lpData != NULL && wcslen((wchar_t*)lpData) > 0) {
                SendMessageW(hWnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)lpData);
            }
        break;
    }
    return 0;
}

int dir_select_dialog(wchar_t* start_path, wchar_t* path, size_t length) {
    wchar_t buff[MAX_PATH + 1] = { 0 };
    int success      = 0;
    BROWSEINFOW info = {
        .hwndOwner      = NULL,
        .pidlRoot       = NULL,
        .lpfn           = dir_select_callback,
        .pszDisplayName = buff,
        .lpszTitle      = L"Select Search Directory",
        .ulFlags        = BIF_RETURNONLYFSDIRS | BIF_EDITBOX,
        .lParam         = (LPARAM)start_path
    };
    PIDLIST_ABSOLUTE t = SHBrowseForFolderW(&info);
    if(t != NULL) {
        if(SHGetPathFromIDListW(t, buff) == TRUE) {
            success = 1;
            size_t len = wcslen(buff);
            if(len >= length) {
                len = length - 1;
            }
            memcpy(path, buff, len * sizeof(wchar_t));
            path[len] = L'\0';
        }
    }
    // Free the ID list
    CoTaskMemFree(t);
    return success;
}

uint64_t rand64() {
    uint64_t data = 0;
    int      ret  = 0;
    do {
        ret = _rdrand64_step(&data);
    } while(ret != 1);
    return data;
}

uint64_t rand64_less_than(uint64_t value) {
    if(value == 0) {
        return 0;
    }
    uint64_t mask = value - 1;
    mask |= (mask >>  1);
    mask |= (mask >>  2);
    mask |= (mask >>  4);
    mask |= (mask >>  8);
    mask |= (mask >> 16);
    mask |= (mask >> 32);
    uint64_t val = value;
    while(val >= value) {
        val = rand64() & mask;
    }
    return val;
}

#define CH_DOT 26
#define CH_ALL 27

// This sets all other characters other than the period/dot
// a-z and A-Z to CH_ALL
static unsigned char categorize_wchar(wchar_t c) {
    if(c > L'z' || c < L'.') {
        return CH_ALL;
    }
    // Fold lower and upper case
    c &= 0x5f;
    if(c == 0x0e) {
        return CH_DOT;
    }
    if(c > L'Z' || c < L'A') {
        return CH_ALL;
    }
    return c - L'A';
}

// Just a table of the following extensions already pre categorized
static const unsigned char exts[11][5] = {
    { 0x1a, 0x1a, 0x03, 0x08, 0x01 }, // extension: ..dib
    { 0x1a, 0x09, 0x0f, 0x04, 0x06 }, // extension: .jpeg
    { 0x1a, 0x1a, 0x09, 0x0f, 0x06 }, // extension: ..jpg
    { 0x1a, 0x1a, 0x0f, 0x0d, 0x06 }, // extension: ..png
    { 0x1a, 0x1a, 0x09, 0x0f, 0x04 }, // extension: ..jpe
    { 0x1a, 0x1a, 0x01, 0x0c, 0x0f }, // extension: ..bmp
    { 0x1a, 0x1a, 0x16, 0x03, 0x0f }, // extension: ..wdp
    { 0x1a, 0x09, 0x05, 0x08, 0x05 }, // extension: .jfif
    { 0x1a, 0x1a, 0x06, 0x08, 0x05 }, // extension: ..gif
    { 0x1a, 0x1a, 0x13, 0x08, 0x05 }, // extension: ..tif
    { 0x1a, 0x13, 0x08, 0x05, 0x05 }, // extension: .tiff
};

bool has_img_ext(const wchar_t* name) {
    unsigned char cat[5] = { 0x1a, 0x1a, 0x1a, 0x1a, 0x1a };
    size_t len           = wcslen(name);
    if(len < 4) {
        return false;
    }
    const wchar_t* cur     = &(name[len - 1]);
    unsigned char* cur_cat = cat + 4;
    while(*cur != L'.' && cur >= name && cur_cat >= cat) {
        *cur_cat = categorize_wchar(*cur);
        cur--;
        cur_cat--;
    }
    // check if the characters category stream matches
    // one of extensions
    for(int i = 0; i < 11; i++) {
        if(memcmp(cat, exts[i], 5) == 0) {
            return true;
        }
    }
    return false;
}