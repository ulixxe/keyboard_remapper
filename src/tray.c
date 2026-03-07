#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include "tray.h"
#include "resource.h"
#include "keyboard_remapper.h"
#include "config.h"
#include "debug.h"

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------
#define WM_TRAYICON     (WM_APP + 1)
#define TRAY_ICON_ID    1001

#define ID_TRAY_PAUSE   2001
#define ID_TRAY_RESET   2002
#define ID_TRAY_EDIT    2003
#define ID_TRAY_RESTART 2004
#define ID_TRAY_DEBUG   2005
#define ID_TRAY_ABOUT   2006
#define ID_TRAY_EXIT    2007

// -----------------------------------------------------------------------------
// globals
// -----------------------------------------------------------------------------
static HICON g_hIconNormal  = NULL;
static HICON g_hIconPaused  = NULL;
static NOTIFYICONDATA g_nid;
static UINT WM_TASKBARCREATED;
static int g_pause = 0;

// -----------------------------------------------------------------------------
// helpers
// -----------------------------------------------------------------------------
static HICON LoadSmallIcon(HINSTANCE hInst, int resId) {
  return (HICON)LoadImage(hInst,
                          MAKEINTRESOURCE(resId),
                          IMAGE_ICON,
                          GetSystemMetrics(SM_CXSMICON),
                          GetSystemMetrics(SM_CYSMICON),
                          0);
}

static BOOL IsLightMode(void) {
  DWORD value = 1;
  DWORD size  = sizeof(value);
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegQueryValueExW(hKey, L"SystemUsesLightTheme", NULL, NULL, (LPBYTE)&value, &size);
    RegCloseKey(hKey);
  }
  return (value != 0);
}

static void LoadThemeIcons(HINSTANCE hInst) {
  BOOL light = IsLightMode();
  if (g_hIconNormal) { DestroyIcon(g_hIconNormal); g_hIconNormal = NULL; }
  if (g_hIconPaused) { DestroyIcon(g_hIconPaused); g_hIconPaused = NULL; }

  if (light) {
    g_hIconNormal = LoadSmallIcon(hInst, IDI_TRAYICON_LIGHT);
    g_hIconPaused = LoadSmallIcon(hInst, IDI_TRAYICON_PAUSED_LIGHT);
  } else {
    g_hIconNormal = LoadSmallIcon(hInst, IDI_TRAYICON_DARK);
    g_hIconPaused = LoadSmallIcon(hInst, IDI_TRAYICON_PAUSED_DARK);
  }
}

static void Tray_Add(HWND hwnd, HICON hIcon, const char *tooltip) {
  ZeroMemory(&g_nid, sizeof(g_nid));
  g_nid.cbSize = sizeof(NOTIFYICONDATA);
  g_nid.hWnd = hwnd;
  g_nid.uID = TRAY_ICON_ID;
  g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = hIcon;
  lstrcpynA(g_nid.szTip, tooltip, sizeof(g_nid.szTip));
  Shell_NotifyIcon(NIM_ADD, &g_nid);
}

static void Tray_Modify(HICON hIcon, const char *tooltip) {
  g_nid.hIcon  = hIcon;
  lstrcpynA(g_nid.szTip, tooltip, sizeof(g_nid.szTip));
  Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

static void Tray_Delete(void) {
  Shell_NotifyIconA(NIM_DELETE, &g_nid);
}

// -----------------------------------------------------------------------------
// menu
// -----------------------------------------------------------------------------
static void ShowContextMenu(HWND hwnd) {
  POINT pt;
  GetCursorPos(&pt);

  HMENU hMenu = CreatePopupMenu();
  if (!hMenu) return;

  InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING,
              ID_TRAY_PAUSE, g_pause ? L"Resume Remapping" : L"Pause Remapping");
  InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_TRAY_RESET,   L"Reset Status");
  InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_EDIT,    L"Edit Config...");
  InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_TRAY_RESTART, L"Restart");
  InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_STRING,
              ID_TRAY_DEBUG, g_debug ? L"Disable Debug Console" : L"Enable Debug Console");
  InsertMenuW(hMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
  InsertMenuW(hMenu, 6, MF_BYPOSITION | MF_STRING, ID_TRAY_ABOUT, L"About");
  InsertMenuW(hMenu, 7, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT,  L"Exit");

  SetForegroundWindow(hwnd);
  TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
  DestroyMenu(hMenu);
}

static void ShowAboutDialog(HWND hwnd) {
  const char *url = "https://github.com/ulixxe/keyboard_remapper";
  char message[512];
  snprintf(message, sizeof(message),
           "keyboard_remapper %s\n\nProject page:\n%s\n\nOpen project page in your browser?",
           VERSION, url);

  if (MessageBoxA(hwnd, message, "About keyboard_remapper",
                  MB_YESNO | MB_ICONINFORMATION) == IDYES) {
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  }
}

static BOOL Restart (HWND hwnd) {
  wchar_t exePath[MAX_PATH];
  DWORD len = GetModuleFileNameW(NULL, exePath, MAX_PATH);
  if (len == 0 || len == MAX_PATH) {
    MessageBoxW(hwnd, L"Cannot locate own .exe – config not reloaded.",
                L"Reload Failed", MB_OK | MB_ICONERROR);
    return FALSE;
  }
  STARTUPINFOW si = { sizeof(si) };
  PROCESS_INFORMATION pi;
  CloseHandle(g_singleInstanceMutex);
  g_singleInstanceMutex = NULL;
  if (CreateProcessW(exePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    PostQuitMessage(0);
  } else {
    wchar_t buf[256];
    swprintf_s(buf, 256, L"Failed to relaunch:\nError %lu", GetLastError());
    MessageBoxW(hwnd, buf, L"Reload Failed", MB_OK|MB_ICONERROR);
  }
  return TRUE;
}

// -----------------------------------------------------------------------------
// window proc
// -----------------------------------------------------------------------------
static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  HINSTANCE hInst = GetModuleHandle(NULL);
  if (uMsg == WM_TASKBARCREATED) {
    LoadThemeIcons(hInst);
    Tray_Add(hwnd,
             g_pause ? g_hIconPaused : g_hIconNormal,
             g_pause ? "keyboard_remapper\n(paused)" : "keyboard_remapper");
    return 0;
  }
  switch (uMsg) {
  case WM_TRAYICON:
    if (wParam == TRAY_ICON_ID) {
      switch (LOWORD(lParam)) {
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
        ShowContextMenu(hwnd);
        return 0;
      case WM_LBUTTONDBLCLK:
        ShowAboutDialog(hwnd);
        return 0;
      }
    }
    break;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case ID_TRAY_PAUSE:
      if (!g_pause) {
        pause();
        g_pause = 1;
        Tray_Modify(g_hIconPaused, "keyboard_remapper\n(paused)");
      } else {
        resume();
        g_pause = 0;
        Tray_Modify(g_hIconNormal, "keyboard_remapper");
      }
      return 0;

    case ID_TRAY_RESET:
      unlock_all(&g_input_buffer);
      if (!input_buffer_empty(&g_input_buffer))
        SetEvent(g_hEvent);
      debug_file_cleanup();
      rehook();
      g_pause = 0;
      g_status = (struct Status){0};
      profiler_init(&g_profiler_timer);
      Tray_Modify(g_hIconNormal, "keyboard_remapper");
      return 0;

    case ID_TRAY_EDIT:
      ShellExecuteW(NULL, L"open", g_config_path, NULL, NULL, SW_SHOWNORMAL);
      return 0;

    case ID_TRAY_RESTART:
      if (Restart(hwnd))
        return 0;
      break;

    case ID_TRAY_DEBUG:
      toggle_debug_mode_async();
      return 0;

    case ID_TRAY_ABOUT:
      ShowAboutDialog(hwnd);
      return 0;

    case ID_TRAY_EXIT:
      PostQuitMessage(0);
      return 0;
    }
    break;

  case WM_SETTINGCHANGE:
    LoadThemeIcons(hInst);
    Tray_Modify(g_pause ? g_hIconPaused : g_hIconNormal,
                g_pause ? "keyboard_remapper\n(paused)" : "keyboard_remapper");
    return 0;

  case WM_THEMECHANGED:
    // Fallback for theme change notifications (covers some Windows versions)
    LoadThemeIcons(hInst);
    Tray_Modify(g_pause ? g_hIconPaused : g_hIconNormal,
                g_pause ? "keyboard_remapper\n(paused)" : "keyboard_remapper");
    return 0;

  case WM_DESTROY:
    Tray_Delete();
    return 0;
  }

  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// ---------- DPI awareness with graceful fallback ----------
static void EnableBestDpiAwareness(void) {
  // Try Per-Monitor v2 if available (Win10+). Use GetProcAddress to avoid link-time dependency.
  HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
  if (hUser32) {
    typedef BOOL (WINAPI *SetPDAwarenessCtxFn)(HANDLE);
    SetPDAwarenessCtxFn pSet = (SetPDAwarenessCtxFn)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
    if (pSet) {
      // Attempt PMv2; if it fails, fall back below
      if (pSet((HANDLE)-4)) { // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        return;
      }
    }
  }
  // Fallback (older systems)
  SetProcessDPIAware();
}

// -----------------------------------------------------------------------------
// public
// -----------------------------------------------------------------------------
HWND Tray_Init(void) {
  EnableBestDpiAwareness();

  HINSTANCE hInst = GetModuleHandle(NULL);
  const char CLASS_NAME[] = "HiddenTrayWindow";

  WNDCLASSEXA wc = {0};
  wc.cbSize        = sizeof(wc);
  wc.lpfnWndProc   = TrayWndProc;
  wc.hInstance     = hInst;
  wc.lpszClassName = CLASS_NAME;
  if (!RegisterClassExA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    return NULL;
  }

  HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Hidden Tray Window",
                             0, 0, 0, 0, 0,
                             NULL, NULL, hInst, NULL);
  if (!hwnd) return NULL;

  WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

  LoadThemeIcons(hInst);
  Tray_Add(hwnd, g_hIconNormal, "keyboard_remapper");

  return hwnd;
}

void Tray_Cleanup(HWND hwnd) {
  if (hwnd) {
    DestroyWindow(hwnd);
    hwnd = NULL;
  }
  if (g_hIconNormal) { DestroyIcon(g_hIconNormal); g_hIconNormal = NULL; }
  if (g_hIconPaused) { DestroyIcon(g_hIconPaused); g_hIconPaused = NULL; }
}
