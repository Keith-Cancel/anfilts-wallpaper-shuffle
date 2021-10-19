#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <Windowsx.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "tray-dialogs.h"
#include "resource.h"
#include "utils.h"

#define TRAY_EVENT   (WM_USER + 1)
#define WINDOW_CLASS "WALLPAPER_SHUFFLE_WINDOW_CLASS"

typedef struct win_res_s {
    HMENU           menu[2];
    wchar_t*        buffer[2];
    bool            diag_about;
    bool            diag_setting;
    UpdateCallBacks call_backs;
} wResources;

void menus_toggle(bool enabled, wResources* res) {
    if(enabled) {
        CheckMenuItem(res->menu[1],  0, MF_BYPOSITION | MF_CHECKED);
        EnableMenuItem(res->menu[1], 0, MF_BYPOSITION | MF_GRAYED);
        CheckMenuItem(res->menu[1],  1, MF_BYPOSITION | MF_UNCHECKED);
        EnableMenuItem(res->menu[1], 1, MF_BYPOSITION | MF_ENABLED);
    } else {
        CheckMenuItem(res->menu[1],  0, MF_BYPOSITION | MF_UNCHECKED);
        EnableMenuItem(res->menu[1], 0, MF_BYPOSITION | MF_ENABLED);
        CheckMenuItem(res->menu[1],  1, MF_BYPOSITION | MF_CHECKED);
        EnableMenuItem(res->menu[1], 1, MF_BYPOSITION | MF_GRAYED);
    }
}


static BOOL CALLBACK about_func(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    // get extra info about the windows in the portion of the window user struct
    wResources* res = (wResources*)GetWindowLongPtr(hDlg, DWLP_USER);
    if(res == NULL && message != WM_INITDIALOG) {
        return FALSE;
    }
	switch (message) {
	    case WM_INITDIALOG:
            // Store our extra info in the user pointer in the dialog
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
		    return TRUE;
        case WM_CLOSE:
            // make certain to set open flag to false
            res->diag_about = false;
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
	    case WM_COMMAND:
            // Close the about window when OK or Cancel is clicked
		    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                SendMessage(hDlg, WM_CLOSE, 0, 0);
            }
            return TRUE;
		break;
	}
	return FALSE;
}

void settings_initialize(HWND hDlg, wResources* res) {
    // Make Visible on the taskbar
    LONG style = GetWindowLongA(hDlg, GWL_EXSTYLE);
    style |= WS_EX_APPWINDOW;
    SetWindowLong(hDlg, GWL_EXSTYLE, style);

    // Initialize timer frequency controls
    HWND combo = GetDlgItem(hDlg, FREQ_UNI);
    ComboBox_AddString(combo, "Seconds");
    ComboBox_AddString(combo, "Minutes");
    ComboBox_AddString(combo, "Hours");
    ComboBox_AddString(combo, "Days");
    if(res->call_backs.get_freq_sec != NULL) {
        int freq  = res->call_backs.get_freq_sec(res->call_backs.user_param);
        int index = 0;
        int div   = 1;
        // is even multiple of minutes
        if((freq % 60) == 0) {
            index = 1;
            div   = 60;
        }
        // is even multiple of hours?
        if((freq % (60 * 60)) == 0) {
            index = 2;
            div   = 60 * 60;
        }
        // is even multiple of days?
        if((freq % (24 * 60 * 60)) == 0) {
            index = 3;
            div   = 24 * 60 * 60;
        }
        ComboBox_SetCurSel(combo, index);
        SetDlgItemInt(hDlg, FREQ_TXT, freq / div, FALSE);
    }
    // Initialize Path
    if(res->call_backs.get_path != NULL) {
        res->call_backs.get_path(res->call_backs.user_param, res->buffer[0], MAX_PATH);
        ExpandEnvironmentStringsW(res->buffer[0], res->buffer[1], MAX_PATH);
        SetDlgItemTextW(hDlg, SRCH_TXT, res->buffer[1]);
    }
    // Initialize enabled checkbox
    UINT checked = BST_UNCHECKED;
    if(res->call_backs.is_enabled != NULL && res->call_backs.is_enabled(res->call_backs.user_param)) {
        checked = BST_CHECKED;
    }
    CheckDlgButton(hDlg, TIMER_ON, checked);

    checked = BST_UNCHECKED;
    if(res->call_backs.is_startup_on != NULL && res->call_backs.is_startup_on(res->call_backs.user_param)) {
        checked = BST_CHECKED;
    }
    CheckDlgButton(hDlg, BOOT_ON, checked);
}

void settings_save(HWND hDlg, wResources* res) {
    // Save freqency info
    const int units[] = { 1, 60, 60 * 60, 24 * 60 * 60 };
    HWND combo        = GetDlgItem(hDlg, FREQ_UNI);
    int combo_index   = SendMessage(combo, CB_GETCURSEL, 0, 0);

    if(combo_index < 0) {
        combo_index = 0;
    }
    if(combo_index > 3) {
        combo_index = 3;
    }
    BOOL success;
    uint64_t time_amt = GetDlgItemInt(hDlg, FREQ_TXT, &success, FALSE);
    if(success == FALSE) {
        MessageBox(NULL, "The time value is too large!", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if(time_amt == 0) {
        MessageBox(NULL, "The time value must be non-zero!", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    time_amt *= units[combo_index];
    if(time_amt > INT_MAX) {
        MessageBox(NULL, "The time value is too large!", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if(res->call_backs.set_freq_sec != NULL) {
        res->call_backs.set_freq_sec(res->call_backs.user_param, time_amt);
    }
    // Save Path info
    GetDlgItemTextW(hDlg, SRCH_TXT, res->buffer[0], MAX_PATH);
    DWORD len = ExpandEnvironmentStringsW(res->buffer[0], res->buffer[1], MAX_PATH);
    // includes null term so subtract 1
    if(len > 0) {
        len--;
    }
    switch(dir_exists(res->buffer[1])) {
        case PATH_EXIST:
            break;
        case PATH_NO_EXIST:
            MessageBox(NULL, "The given path does not exist!", "Info", MB_OK | MB_ICONINFORMATION);
            return;
        case PATH_NO_PERM:
            MessageBox(NULL, "Do not have permission to access the given path!", "Info", MB_OK | MB_ICONINFORMATION);
            return;
        case PATH_NOT_DIR:
            MessageBox(NULL, "The given path is not a directory!", "Info", MB_OK | MB_ICONINFORMATION);
            return;
        default:
            MessageBox(NULL, "Error locating path!", "Warning!", MB_OK | MB_ICONWARNING);
            return;
    }
    if(res->call_backs.set_path != NULL) {
        if(res->call_backs.set_path(res->call_backs.user_param, res->buffer[1]) != len) {
            MessageBox(NULL, "The given path is too long!", "Info", MB_OK | MB_ICONINFORMATION);
            return;
        }
    }
    // Save enabled check
    UINT checked = IsDlgButtonChecked(hDlg, TIMER_ON);
    bool enable  = checked == BST_CHECKED;
    if(res->call_backs.set_enable != NULL) {
        res->call_backs.set_enable(res->call_backs.user_param, enable);
        menus_toggle(enable, res);
    }

    // Save startup info
    checked = IsDlgButtonChecked(hDlg, BOOT_ON);
    if(checked == BST_CHECKED) {
        if(res->call_backs.enable_startup != NULL) {
            res->call_backs.enable_startup(res->call_backs.user_param);
        }
    } else {
        if(res->call_backs.disable_startup != NULL) {
            res->call_backs.disable_startup(res->call_backs.user_param);
        }
    }
    SendMessage(hDlg, WM_CLOSE, 0, 0);
}

BOOL settings_handle_command(HWND hDlg, wResources* res, int ctrl_id, int notifcation_code, HANDLE hCtrl) {
    switch(ctrl_id) {
        case SRCH_DIR: {
                GetDlgItemTextW(hDlg, SRCH_TXT, res->buffer[0], MAX_PATH);
                ExpandEnvironmentStringsW(res->buffer[0], res->buffer[1], MAX_PATH);
                if(dir_select_dialog(res->buffer[1], res->buffer[1], MAX_PATH)) {
                    SetDlgItemTextW(hDlg, SRCH_TXT, res->buffer[1]);
                }
            }
            break;
        case IDOK:
            settings_save(hDlg, res);
            break;
        case IDCANCEL:
            SendMessage(hDlg, WM_CLOSE, 0, 0);
            break;
    }
    return TRUE;
}

// return FALSE if we do not process a message
static BOOL CALLBACK settings_func(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    wResources* res = (wResources*)GetWindowLongPtr(hDlg, DWLP_USER);
    if(res == NULL && message != WM_INITDIALOG) {
        return FALSE;
    }
    int ctrl_id = LOWORD(wParam);
    int no_code = HIWORD(wParam);
    switch (message) {
	    case WM_INITDIALOG:
            settings_initialize(hDlg, (wResources*)lParam);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
            return TRUE;
        case WM_CLOSE:
            // make certain to set open flag to false
            res->diag_setting = false;
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
	    case WM_COMMAND:
            return settings_handle_command(hDlg, res, ctrl_id, no_code, (HANDLE)lParam);
	}
    return FALSE;
}

static int command(HWND hWnd, wResources* res, int cmd_id, int cmd_event) {
    switch(cmd_id) {
        case MENU_ITEM_ABOUT:
            // Even though MSDN docs say NULL for first paramter should use the executable ???
            // Some reason the icon resource was not loading properly in the about dialog
            // So just GetModuleHandle since that gets the executable instance and it works
            if(res->diag_about) {
                MessageBox(NULL, "The about window is already open!", "Info", MB_OK | MB_ICONINFORMATION);
            } else {
                res->diag_about = true;
                DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(DIA_ABOUT), hWnd, (DLGPROC)about_func, (LPARAM)res);
            }
            break;
        case MENU_ITEM_SETTINGS:
            if(res->diag_setting) {
                MessageBox(NULL, "The settings window is already open!", "Info", MB_OK | MB_ICONINFORMATION);
            } else {
                res->diag_setting = true;
                DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(DIA_SETTING), hWnd, (DLGPROC)settings_func, (LPARAM)res);
            }
            break;
        case MENU_ITEM_SHUFFLE:
            if(res->call_backs.run_now != NULL) {
                res->call_backs.run_now(res->call_backs.user_param);
            }
            break;
        case MENU_ITEM_ENABLE:
            if(res->call_backs.set_enable == NULL || res->call_backs.is_enabled == NULL) {
                break;
            }
            if(res->call_backs.is_enabled(res->call_backs.user_param)) {
                MessageBox(NULL, "Wallpaper shuffling already enabled!", "Info", MB_OK | MB_ICONINFORMATION);
                break;
            }
            res->call_backs.set_enable(res->call_backs.user_param, true);
            menus_toggle(true, res);
            break;
        case MENU_ITEM_DISABLE:
            if(res->call_backs.set_enable == NULL || res->call_backs.is_enabled == NULL) {
                break;
            }
            if(!res->call_backs.is_enabled(res->call_backs.user_param)) {
                MessageBox(NULL, "Wallpaper shuffling already disabled!", "Info", MB_OK | MB_ICONINFORMATION);
                break;
            }
            res->call_backs.set_enable(res->call_backs.user_param, false);
            menus_toggle(false, res);
            break;
        case MENU_ITEM_EXIT:
            PostQuitMessage(0);
            break;
        default:
            return 1;
    }
    return 0;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    POINT pt = { 0 };
    wResources* res = (wResources*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch(msg) {
        case TRAY_EVENT:
            // Check if it is a right click and only use the menu if not NULL
            if(LOWORD(lParam) == WM_RBUTTONDOWN && res->menu[1]) {
                GetCursorPos(&pt);
                TrackPopupMenu(res->menu[1], TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_COMMAND:
            if(!command(hWnd, res, LOWORD(wParam), HIWORD(wParam))) {
                break;
            }
            // fall through
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static bool register_class(HINSTANCE hInstance) {
    INITCOMMONCONTROLSEX icc;
    // Initialise common controls.
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);
    char wClassName[] = WINDOW_CLASS;

    // Do not call DestroyIcon on shared incon's
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroyicon#Remarks
    WNDCLASSEX wClass = {
        .cbSize        = sizeof(WNDCLASSEX),
        .lpfnWndProc   = WndProc,
        .lpszClassName = wClassName,
        .hInstance     = hInstance,
        .cbWndExtra    = sizeof(LONG_PTR),
        .style         = CS_HREDRAW | CS_VREDRAW,
        .hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_MAIN)),
        .hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_TINY))
    };
    return (RegisterClassEx(&wClass) != 0);
}

HWND create_tray_window(UpdateCallBacks call_backs) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if(!register_class(hInstance)) {
        return NULL;
    }
    wResources* res = malloc(sizeof(wResources));
    if(res == NULL) {
        UnregisterClassA(WINDOW_CLASS, hInstance);
        return NULL;
    }
    memset(res, 0, sizeof(wResources));
    size_t   buf_sz  = sizeof(wchar_t) * (MAX_PATH * 2 + 2);
    wchar_t* buffers = malloc(buf_sz);
    if(buffers == NULL) {
        free(res);
        UnregisterClassA(WINDOW_CLASS, hInstance);
        return NULL;
    }
    memset(buffers, 0, buf_sz);
    memcpy(&(res->call_backs), &call_backs, sizeof(UpdateCallBacks));
    // Initialize Dialog Flags
    res->diag_about   = false;
    res->diag_setting = false;
    // Load the tray menu from the exes resource section
    res->menu[0] = LoadMenu(hInstance, MAKEINTRESOURCE(MENU_TRAY));
    if(res->menu[0] != NULL) {
        // The sub-menu is the pop-menu we defined in the resource
        // file of an other wise empty menu.
        res->menu[1] = GetSubMenu(res->menu[0], 0);
    }
    res->buffer[0] = buffers;
    res->buffer[1] = &(buffers[MAX_PATH + 1]);
    // Now create window
    HWND hWnd = CreateWindowA(
        WINDOW_CLASS, "Wallpaper Shuffle", 0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );
    if(hWnd == NULL) {
        UnregisterClassA(WINDOW_CLASS, hInstance);
        return NULL;
    }
    // set enable toggles to inital state
    menus_toggle(res->call_backs.is_enabled(res->call_backs.user_param), res);

    // Add our info struct to the user section of the window
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)res);
    return hWnd;
}

bool init_tray(HWND hWnd) {
    // Use the icon in the resource section of our executable for tray icon

    HINSTANCE hInstance = GetModuleHandle(NULL);
    HICON     hIcon     = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_TRAY));
    NOTIFYICONDATAW nidApp = {
        .cbSize = sizeof(NOTIFYICONDATAW),
        .hWnd   = hWnd,
        .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP,
        .uID    = ICON_TRAY,
        .hIcon  = hIcon,
        .uCallbackMessage = TRAY_EVENT
    };
    // as well as set the tool tip text
    LoadStringW(hInstance, STR_TRAY_TIP, nidApp.szTip, sizeof(nidApp.szTip));
    // Add our window, icon and tool tip to the system tray
    return Shell_NotifyIconW(NIM_ADD, &nidApp);
}

void destroy_tray_window(HWND hWnd) {
    if(hWnd == NULL) {
        return;
    }
    wResources* res = (wResources*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    // Don't destroy resource's from the executables resource section
    /*for(int i = 0; i < 2; i++) {
        DestroyMenu(res->menu[i]);
    }*/
    free(res->buffer[0]);
    free(res);
    DestroyWindow(hWnd);
    UnregisterClassA(WINDOW_CLASS, GetModuleHandle(NULL));
}