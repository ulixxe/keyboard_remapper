#ifndef KEYBOARD_REMAPPER_H
#define KEYBOARD_REMAPPER_H

#include <windows.h> // HANDLE, MAX_PATH
#include "input_buffer.h"

#define VERSION "1.2.1"

// A semi random value used to identify inputs generated
// by Dual Key Remap. Ideally high to minimize chances of a collision
// with a real pointer used by another application.
// Note: This approach is what AHK used, we should a different key id
// from them to avoid collisions.
#define INJECTED_KEY_ID (0xFFC3CFD7 & 0xFFFFFF00)

enum Direction {
  UP,
  DOWN,
};

void send_input(int scan_code,
                int virt_code,
                enum Direction direction,
                int remap_id,
                struct InputBuffer *input_buffer);
void rehook();
void pause();
void resume();
void toggle_debug_mode_async();
extern HANDLE g_hEvent;
extern HANDLE g_hTimerQueue;
extern HANDLE g_singleInstanceMutex;
extern wchar_t g_config_path[MAX_PATH];
extern struct InputBuffer g_input_buffer;
extern int g_input_buffer_max;
extern int g_mouse_blocked_events;
extern int g_mouse_passthrough_events;
extern int g_keyboard_blocked_events;
extern int g_keyboard_passthrough_events;
extern g_status_lines;

#endif // KEYBOARD_REMAPPER_H
