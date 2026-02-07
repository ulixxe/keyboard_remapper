#include <windows.h>
#include <stdio.h>
#include "keyboard_remapper.h"
#include "resource.h"
#include "input_buffer.h"
#include "keys.h"
#include "config.h"
#include "debug.h"
#include "remap.h"
#include "mouse.h"
#include "tray.h"

// Globals
static HHOOK g_keyboard_hook;
static HHOOK g_mouse_hook;
HANDLE g_hEvent;
HANDLE g_hTimerQueue = NULL;
HANDLE g_singleInstanceMutex = NULL;
wchar_t g_config_path[MAX_PATH];
struct InputBuffer g_input_buffer;
int g_input_buffer_max = 0;
int g_mouse_blocked_events = 0;
int g_mouse_passthrough_events = 0;
int g_keyboard_blocked_events = 0;
int g_keyboard_passthrough_events = 0;
int g_status_lines = 0;

void send_input(int scan_code, int virt_code, enum Direction direction, int remap_id, struct InputBuffer *input_buffer) {
  if (virt_code) {
    uint32_t n, tail;
    int index;
    n = input_buffer_move_prod_head(input_buffer, &tail);
    index = tail & INPUT_BUFFER_MASK;
    if (n == 0) {
      if (g_debug) debug_print(ANSI_RED, "Error: input buffer is full!\n");
      debug_file("Error: input buffer is full!");
      return;
    }
    INPUT *out = &input_buffer->inputs[index];
    //ZeroMemory(out, sizeof(INPUT));

    out->type = INPUT_KEYBOARD;
    out->ki.time = 0;
    out->ki.dwExtraInfo = (ULONG_PTR)(INJECTED_KEY_ID | remap_id);
    out->ki.wScan = scan_code;
    out->ki.wVk = ((g_scancode && scan_code != 0x00) ? 0 : virt_code);
    // Per MS Docs: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-keybd_even
    // we need to flag whether "the scan code was preceded by a prefix byte having the value 0xE0 (224)"
    int is_extended_key = (scan_code >> 8) == 0xE0;
    out->ki.dwFlags = (direction == UP ? KEYEVENTF_KEYUP : 0) |
      (is_extended_key ? KEYEVENTF_EXTENDEDKEY : 0) |
      ((g_scancode && scan_code != 0x00) ? KEYEVENTF_SCANCODE : 0);
    input_buffer_update_tail(&input_buffer->prod, tail, n);
  } else {
    mouse_emulation(scan_code, direction, remap_id, &g_input_buffer);
  }
}

static LRESULT CALLBACK mouse_callback(int msg_code, WPARAM w_param, LPARAM l_param) {
  int block_input = 0;

  // Per MS docs we should only act for HC_ACTION's
  if (msg_code == HC_ACTION) {
    MSLLHOOKSTRUCT *data = (MSLLHOOKSTRUCT *)l_param;
    int is_injected = ((LLMHF_INJECTED & data->flags) && data->dwExtraInfo != 0x00) ? 1 : 0;
    switch (w_param) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_MOUSEWHEEL:
      // Since no key corresponds to the mouse inputs; use a dummy input
      block_input = handle_input(
                                 w_param,
                                 MOUSE_DUMMY_VK,
                                 DOWN,
                                 data->time,
                                 is_injected,
                                 data->flags,
                                 data->dwExtraInfo,
                                 &g_input_buffer);
    }

    if (block_input == -1) {
      uint32_t n, tail;
      int index;
      n = input_buffer_move_prod_head(&g_input_buffer, &tail);
      index = tail & INPUT_BUFFER_MASK;
      if (n == 0) {
        if (g_debug) debug_print(ANSI_RED, "Error: input buffer is full!\n");
        debug_file("Error: input buffer is full!");
        return 1;
      }
      INPUT *out = &g_input_buffer.inputs[index];
      //ZeroMemory(out, sizeof(INPUT));

      out->type = INPUT_MOUSE;
      out->mi.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID;

      switch (w_param) {
      case WM_LBUTTONDOWN:
        out->mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
        break;
      case WM_RBUTTONDOWN:
        out->mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
        break;
      case WM_MBUTTONDOWN:
        out->mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
        break;
      case WM_XBUTTONDOWN:
        out->mi.dwFlags |= MOUSEEVENTF_XDOWN;
        out->mi.mouseData = data->mouseData;
        break;
      case WM_MOUSEWHEEL:
        out->mi.dwFlags |= MOUSEEVENTF_WHEEL;
        out->mi.mouseData = ((int)data->mouseData)>>16;
        break;
      }
      input_buffer_update_tail(&g_input_buffer.prod, tail, n);
    }
    if (!input_buffer_empty(&g_input_buffer))
      SetEvent(g_hEvent);
  }

  if (block_input) {
    g_mouse_blocked_events++;
    return 1;
  } else {
    g_mouse_passthrough_events++;
    return CallNextHookEx(NULL, msg_code, w_param, l_param);
  }
}

static LRESULT CALLBACK keyboard_callback(int msg_code, WPARAM w_param, LPARAM l_param) {
  int block_input = 0;
    
  // Per MS docs we should only act for HC_ACTION's
  if (msg_code == HC_ACTION) {
    KBDLLHOOKSTRUCT *data = (KBDLLHOOKSTRUCT *)l_param;
    enum Direction direction = (LLKHF_UP & data->flags) ? UP : DOWN;
    int is_injected = (LLKHF_INJECTED & data->flags) ? 1 : 0;
    block_input = handle_input(
                               data->scanCode,
                               data->vkCode,
                               direction,
                               data->time,
                               is_injected,
                               data->flags,
                               data->dwExtraInfo,
                               &g_input_buffer
                               );

    if (block_input == -1) {
      send_input(data->scanCode, data->vkCode, direction, 0, &g_input_buffer);
    }
    if (!input_buffer_empty(&g_input_buffer))
      SetEvent(g_hEvent);
  }

  if (block_input) {
    g_keyboard_blocked_events++;
    return 1;
  } else {
    g_keyboard_passthrough_events++;
    return CallNextHookEx(NULL, msg_code, w_param, l_param);
  }
}

static DWORD WINAPI send_input_thread(LPVOID arg) {
  uint32_t n, tail;
  int index;
  struct InputBuffer *input_buffer = (struct InputBuffer *)arg;
  while (1) {
    WaitForSingleObject(g_hEvent, INFINITE);
    ResetEvent(g_hEvent);
    while (!input_buffer_empty(input_buffer)) {
      n = input_buffer_move_cons_head(input_buffer, -2, &tail);
      if (g_input_buffer_max < n) g_input_buffer_max = n;
      index = tail & INPUT_BUFFER_MASK;
      if (n > 0) {
        SendInput(n, &input_buffer->inputs[index], sizeof(INPUT));
        input_buffer_clear_cons(input_buffer, n);
        input_buffer_update_tail(&input_buffer->cons, tail, n);
      }
    }
  }
}

static void enable_ansi_support() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(hConsole, &mode);
  mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING |
    ENABLE_PROCESSED_OUTPUT |
    ENABLE_WRAP_AT_EOL_OUTPUT;
  SetConsoleMode(hConsole, mode);
}

static void close_all();

// Console control handler - called by Windows when console is being closed
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  switch (ctrl_type) {
  case CTRL_BREAK_EVENT:      // User pressed Ctrl+Break
  case CTRL_C_EVENT:          // User pressed Ctrl+C
    toggle_debug_mode_async();
    return TRUE;
  case CTRL_CLOSE_EVENT:      // User clicked X button
  case CTRL_LOGOFF_EVENT:     // System is logging off
  case CTRL_SHUTDOWN_EVENT:   // System is shutting down
    unlock_all(&g_input_buffer);
    if (!input_buffer_empty(&g_input_buffer))
      SetEvent(g_hEvent);
    debug_file_cleanup();
    Sleep(1000);
    return FALSE;
  default:
    return FALSE;
  }
}

void create_console() {
  if (AllocConsole()) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    enable_ansi_support();
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
  }
}

void destroy_console() {
  if (GetConsoleWindow() == NULL)
    return;
  SetConsoleCtrlHandler(console_ctrl_handler, FALSE);
  fclose(stdout);
  fclose(stderr);
  FreeConsole();
}

struct DebugSetupParams {
  int enable;
};

static DWORD WINAPI debug_setup_thread(LPVOID arg) {
  struct DebugSetupParams *params = (struct DebugSetupParams *)arg;
  int enable = params->enable;
  free(params);

  if (enable) {
    create_console();
    printf("%s-- DEBUG MODE --%s\n", ANSI_GREEN, ANSI_RESET);
    g_status_lines = print_status(0, 1);
    g_debug = 1;
  } else {
    g_debug = 0;
    if (g_hDebugTimer)
      DeleteTimerQueueTimer(g_hTimerQueue, g_hDebugTimer, INVALID_HANDLE_VALUE);
    g_hDebugTimer = NULL;
    destroy_console();
  }

  g_log_last_packet_size = 0;
  return 0;
}

void toggle_debug_mode_async() {
  struct DebugSetupParams *params = malloc(sizeof(struct DebugSetupParams));
  if (!params) return;

  params->enable = !g_debug;

  HANDLE threadHandle = CreateThread(
                                     NULL,               // Default security attributes
                                     0,                  // Default stack size
                                     debug_setup_thread, // Thread function
                                     params,             // Thread parameter
                                     0,                  // Default creation flags
                                     NULL                // Don't need thread ID
                                     );

  if (threadHandle) {
    CloseHandle(threadHandle);
  } else {
    free(params);
  }
}

static int load_config_file(wchar_t *path) {
  FILE *file;

  if (_wfopen_s(&file, path, L"r") > 0) {
    printf("Cannot open configuration file '%ws'. Make sure it is in the same directory as the executable.\n",
           path);
    return 1;
  }

  char line[256];
  int linenum = 1;
  while (fgets(line, sizeof(line), file)) {
    if (load_config_line(line, linenum++)) {
      fclose(file);
      return 1;
    }
  };
  fclose(file);
  return load_config_line(NULL, linenum++);
}

static void put_config_path(wchar_t *path) {
  HMODULE module = GetModuleHandleW(NULL);
  wchar_t exe_path[MAX_PATH];
  GetModuleFileNameW(module, exe_path, MAX_PATH);

  // Find the directory by locating the last backslash
  wchar_t *last_backslash = wcsrchr(exe_path, L'\\');
  if (last_backslash) {
    wcsncpy(path, exe_path, last_backslash - exe_path + 1);
    path[last_backslash - exe_path + 1] = L'\0';
  } else {
    wcscpy(path, L"");
  }
  wcscat(path, L"config.txt");
}

void rehook() {
  if (g_keyboard_hook) UnhookWindowsHookEx(g_keyboard_hook);
  if (g_mouse_hook) UnhookWindowsHookEx(g_mouse_hook);
  g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_callback, NULL, 0);
  g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_callback, NULL, 0);
  if (!g_hTimerQueue) g_hTimerQueue = CreateTimerQueue();
  DEBUG(1, debug_print(ANSI_RED, "Rehooked!\n"));
}

void pause() {
  unlock_all(&g_input_buffer);
  if (!input_buffer_empty(&g_input_buffer))
    SetEvent(g_hEvent);
  debug_file_cleanup();
  if (g_keyboard_hook) UnhookWindowsHookEx(g_keyboard_hook);
  if (g_mouse_hook) UnhookWindowsHookEx(g_mouse_hook);
  g_mouse_timer = 0;
  if (g_hTimerQueue) {
    if (g_hMouseTimer) DeleteTimerQueueTimer(g_hTimerQueue, g_hMouseTimer, INVALID_HANDLE_VALUE);
    g_hMouseTimer = NULL;
    if (g_hDebugTimer) DeleteTimerQueueTimer(g_hTimerQueue, g_hDebugTimer, INVALID_HANDLE_VALUE);
    g_hDebugTimer = NULL;
    DeleteTimerQueueEx(g_hTimerQueue, INVALID_HANDLE_VALUE);
    g_hTimerQueue = NULL;
  }
}

void resume() {
  g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_callback, NULL, 0);
  g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_callback, NULL, 0);
  g_hTimerQueue = CreateTimerQueue();
}

static void close_all() {
  unlock_all(&g_input_buffer);
  if (!input_buffer_empty(&g_input_buffer))
    SetEvent(g_hEvent);
  debug_file_cleanup();
  if (g_keyboard_hook) UnhookWindowsHookEx(g_keyboard_hook);
  if (g_mouse_hook) UnhookWindowsHookEx(g_mouse_hook);
  g_mouse_timer = 0;
  if (g_hTimerQueue) {
    if (g_hMouseTimer) DeleteTimerQueueTimer(g_hTimerQueue, g_hMouseTimer, INVALID_HANDLE_VALUE);
    g_hMouseTimer = NULL;
    if (g_hDebugTimer) DeleteTimerQueueTimer(g_hTimerQueue, g_hDebugTimer, INVALID_HANDLE_VALUE);
    g_hDebugTimer = NULL;
    DeleteTimerQueueEx(g_hTimerQueue, INVALID_HANDLE_VALUE);
    g_hTimerQueue = NULL;
  }
  if (g_hEvent) CloseHandle(g_hEvent);
  free_all();
}

int main() {
  HANDLE threadHandle;
  DWORD threadId;
  HWND hwnd;

  keys_init();

  // Initialization may print errors to stdout, create a console to show that output.
  create_console();
  printf("%s== keyboard_remapper %s ==%s\n", ANSI_GREEN, VERSION, ANSI_RESET);

  g_singleInstanceMutex = CreateMutex(NULL, TRUE, "keyboard_remapper.single-instance");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    HMODULE module = GetModuleHandleW(NULL);
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(module, exe_path, MAX_PATH);
    wchar_t *last_backslash = wcsrchr(exe_path, L'\\');
    wchar_t *exe_name = (last_backslash != NULL) ? (last_backslash + 1) : exe_path;
    printf("%ls is already running!\n", exe_name);
    goto end;
  }

  put_config_path(g_config_path);
  int err = load_config_file(g_config_path);
  if (err) {
    goto end;
  }

  if (g_priority) {
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
      printf("Error setting process priority: %d\n", GetLastError());
      goto end;
    }

    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
      printf("Error setting thread priority: %d\n", GetLastError());
      goto end;
    }
  }

  g_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (g_hEvent == NULL) {
    printf("CreateEvent error: %d\n", GetLastError());
    goto end;
  }
  input_buffer_init(&g_input_buffer);
  threadHandle = CreateThread(NULL, 0, send_input_thread, &g_input_buffer, 0, &threadId);
  if (threadHandle == NULL) {
    printf("Error creating the thread: %d\n", GetLastError());
    goto end;
  }
  g_hTimerQueue = CreateTimerQueue();

  debug_buffer_init(&g_debug_buffer);
  g_debug = g_debug || getenv("DEBUG") != NULL;
  g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_callback, NULL, 0);
  g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_callback, NULL, 0);

  if (g_mouse_hook == NULL || g_keyboard_hook == NULL) {
    printf("Error: Unable to set hooks\n");
    goto end;
  }

  hwnd = Tray_Init();
  if (!hwnd) {
    printf("Failed to initialize tray icon (err=%lu)\n", GetLastError());
    goto end;
  }

  if (g_debug) {
    print_layer_list(g_layer_list);
    printf("%s-- DEBUG MODE --%s\n", ANSI_GREEN, ANSI_RESET);
    g_status_lines = print_status(0, 1);
  } else {
    destroy_console();
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  close_all();
  Tray_Cleanup(hwnd);
  return 0;

 end:
  printf("Press any key to exit...\n");
  getch();
  return 1;
}
