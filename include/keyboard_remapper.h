#ifndef KEYBOARD_REMAPPER_H
#define KEYBOARD_REMAPPER_H

#include <windows.h> // HANDLE, MAX_PATH
#include "input_buffer.h"
#include "config.h"

#define VERSION "1.3.0"

// A semi random value used to identify inputs generated
// by Dual Key Remap. Ideally high to minimize chances of a collision
// with a real pointer used by another application.
// Note: This approach is what AHK used, we should a different key id
// from them to avoid collisions.
#define INJECTED_KEY_ID (0xFFC3CFD7 & ~REMAP_ID_MASK)

enum Direction {
  NONE = 0,
  UP,
  DOWN,
};

struct Status {
  int keyboard_blocked_events;
  int keyboard_passthrough_events;
  int mouse_blocked_events;
  int mouse_passthrough_events;
  int remapped_events;
  int processed_events;
  int filtered_events;
  int input_buffer_max;
  int debug_buffer_max;
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
extern int g_status_lines;
extern struct Status g_status;

#endif // KEYBOARD_REMAPPER_H
