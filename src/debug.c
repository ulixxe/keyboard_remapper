#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "debug.h"
#include "debug_buffer.h"
#include "keyboard_remapper.h"
#include "input_buffer.h"
#include "keys.h"
#include "config.h"
#include "remap.h"

#define DEBUG_INTERVAL_MS 200
#define DEBUG_START_MS (DEBUG_INTERVAL_MS/2)

HANDLE g_hDebugTimer = NULL;
static int g_log_counter = 1;
static int g_log_indent_level = 0;
static int g_log_duplicates = 1;
int g_log_last_packet_size = 0;
int g_debug_buffer_max = 0;
struct DebugBuffer g_debug_buffer;

// Debug Logging
// -------------------------------------
void debug_file(const char *message) {
  if (message == NULL) return;
  FILE *log_file = fopen("debug.log", "a");
  if (log_file == NULL) return;
  time_t now = time(NULL);
  if (now == (time_t)-1) {
    fprintf(log_file, "[unknown time] %s\n", message);
    fclose(log_file);
    return;
  }
  struct tm tm_buf;
  if (localtime_s(&tm_buf, &now) != 0) {
    fprintf(log_file, "[invalid time] %s\n", message);
    fclose(log_file);
    return;
  }
  char timestamp[64];
  if (strftime(timestamp, sizeof(timestamp),
               "%Y-%m-%d %H:%M:%S", &tm_buf) == 0) {
    fprintf(log_file, "[time fmt error] %s\n", message);
  } else {
    fprintf(log_file, "[%s] %s\n", timestamp, message);
  }
  fclose(log_file);
}

void debug_print(const char *color, const char *format, ...) {
  va_list args;
  va_start(args, format);

  printf("%s", color);
  vprintf(format, args);
  printf("%s", ANSI_RESET);

  va_end(args);
}

static VOID CALLBACK debug_callback(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
  uint32_t n, tail;
  ULONGLONG now_ms = GetTickCount64();
  int debug_timer = *(int *)lpParam;
  int duplicate_indent_level = 0;
  char buf[8];
  while (!debug_buffer_empty(&g_debug_buffer)) {
    n = debug_buffer_move_cons_head(&g_debug_buffer, DEBUG_BUFFER_SIZE, &tail);
    if (n > 0) {
      if (g_debug_buffer_max < n) g_debug_buffer_max = n;
      if (debug_timer) {
        uint32_t head = tail + n;
        uint32_t curr = tail;
        while (curr != head) {
          uint32_t packet_size = debug_buffer_max_sequence(&g_debug_buffer, curr, (head - curr) & DEBUG_BUFFER_MASK);
          if (packet_size == 0) {
            packet_size = 1;
            g_log_last_packet_size = 0;
            g_log_duplicates = 1;
          } else if (((head - curr) & DEBUG_BUFFER_MASK) == packet_size &&
              (now_ms - g_debug_buffer.debugs[curr & DEBUG_BUFFER_MASK].key_time < DEBUG_INTERVAL_MS/2)) {
            debug_buffer_revert_cons_head(&g_debug_buffer, curr);
            debug_buffer_update_tail(&g_debug_buffer.cons, tail, (curr - tail) & DEBUG_BUFFER_MASK);
            return;
          } else {
            if ((DEBUG_BUFFER_SIZE - 1 - ((g_debug_buffer.prod.pos.tail - curr) & DEBUG_BUFFER_MASK)) >= 1 &&
                g_log_last_packet_size == 1) {
              if (debug_buffer_compare(&g_debug_buffer, curr - 1, curr, 1)) {
                packet_size = 1;
                g_log_duplicates++;
                printf(ANSI_BACK(1));
                snprintf(buf, sizeof(buf), "x%d", g_log_duplicates);
                duplicate_indent_level = printf("%*s |", 4, buf) - 1;
              } else {
                g_log_duplicates = 1;
              }
            } else if ((DEBUG_BUFFER_SIZE - 1 - ((g_debug_buffer.prod.pos.tail - curr) & DEBUG_BUFFER_MASK)) >= packet_size &&
                       g_log_last_packet_size == packet_size) {
              if (debug_buffer_compare(&g_debug_buffer, curr - packet_size, curr, packet_size)) {
                g_log_duplicates++;
                printf(ANSI_BACK(packet_size));
                snprintf(buf, sizeof(buf), "x%d", g_log_duplicates);
                duplicate_indent_level = printf("%*s |", 4, buf) - 1;
              } else {
                g_log_duplicates = 1;
              }
            } else {
              g_log_duplicates = 1;
            }
            g_log_last_packet_size = packet_size;
          }
          for (uint32_t i = 0; i < packet_size; i++) {
            const struct DebugData *entry = &g_debug_buffer.debugs[(curr + i) & DEBUG_BUFFER_MASK];
            if (i > 0 && g_log_duplicates > 1)
              printf("%*s%s", duplicate_indent_level, "", "|");
            if (entry->data_type == 1) {
              g_log_indent_level = printf("%3d: %+5d ",
                                          entry->id,
                                          entry->time_offset);
              printf("%s", entry->data);
            } else {
              printf("%*s%s", g_log_indent_level, "", entry->data);
            }
          }
          curr = curr + packet_size;
        }
      } else {
        g_log_last_packet_size = 0;
      }
      debug_buffer_update_tail(&g_debug_buffer.cons, tail, n);
    }
  }
}

void log_handle_input_start(int scan_code, int virt_code, enum Direction direction, DWORD time, int is_injected, DWORD flags, ULONG_PTR dwExtraInfo) {
  DWORD time_elapsed = time - g_last_input;
  if (time_elapsed > 9999) time_elapsed = 9999;
  uint32_t n, tail;
  int index;
  n = debug_buffer_move_prod_head(&g_debug_buffer, &tail);
  index = tail & DEBUG_BUFFER_MASK;
  if (n == 0) {
    if (g_debug) debug_print(ANSI_RED, "Error: debug buffer is full!\n");
    debug_file("Error: debug buffer is full!");
    return;
  }
  struct DebugData *out = &g_debug_buffer.debugs[index];
  out->id = g_log_counter++;
  out->key_time = time;
  out->time_offset = time_elapsed;
  out->data_type = 1;
  out->len = snprintf(out->data,
                      sizeof(out->data),
                      "[%s] %s %s (scan:0x%04X virt:0x%02X flags:0x%02X dwExtraInfo:0x%IX)\n",
                      (is_injected && ((dwExtraInfo & 0xFFFFFF00) == INJECTED_KEY_ID)) ? "output" : "input",
                      friendly_virt_code_name(virt_code),
                      (direction == DOWN) ? "DOWN" : "UP",
                      scan_code, // MapVirtualKeyA(virt_code, MAPVK_VK_TO_VSC_EX)
                      virt_code,
                      flags,
                      dwExtraInfo);
  if (out->len >= sizeof(out->data)) {
    out->len = sizeof(out->data) - 1;
  }
  debug_buffer_update_tail(&g_debug_buffer.prod, tail, n);
  if (g_hDebugTimer == NULL) {
    if (!CreateTimerQueueTimer(&g_hDebugTimer, g_hTimerQueue, (WAITORTIMERCALLBACK)debug_callback,
                               &g_debug, DEBUG_START_MS, DEBUG_INTERVAL_MS, 0)) {
      DEBUG(-1, debug_print(ANSI_RED, "CreateTimerQueueTimer failed (%d)\n", GetLastError()));
    }
  }
}

void log_handle_input_end(int scan_code, int virt_code, enum Direction direction, int block_input) {
  if (block_input) {
    uint32_t n, tail;
    int index;
    n = debug_buffer_move_prod_head(&g_debug_buffer, &tail);
    index = tail & DEBUG_BUFFER_MASK;
    if (n == 0) {
      if (g_debug) debug_print(ANSI_RED, "Error: debug buffer is full!\n");
      debug_file("Error: debug buffer is full!");
      return;
    }
    struct DebugData *out = &g_debug_buffer.debugs[index];
    out->id = g_log_counter;
    out->key_time = 0;
    out->time_offset = 0;
    out->data_type = 2;
    out->len = snprintf(out->data,
                        sizeof(out->data),
                        "blocked-input: %s %s\n",
                        friendly_virt_code_name(virt_code),
                        (direction == DOWN) ? "DOWN" : "UP");
    if (out->len >= sizeof(out->data)) {
      out->len = sizeof(out->data) - 1;
    }
    debug_buffer_update_tail(&g_debug_buffer.prod, tail, n);
    if (g_hDebugTimer == NULL) {
      if (!CreateTimerQueueTimer(&g_hDebugTimer, g_hTimerQueue, (WAITORTIMERCALLBACK)debug_callback,
                                 &g_debug, DEBUG_START_MS, DEBUG_INTERVAL_MS, 0)) {
        DEBUG(-1, debug_print(ANSI_RED, "CreateTimerQueueTimer failed (%d)\n", GetLastError()));
      }
    }
  }
}

void log_send_input(char *remap_name, const KeyDef *key, enum Direction direction) {
  uint32_t n, tail;
  int index;
  n = debug_buffer_move_prod_head(&g_debug_buffer, &tail);
  index = tail & DEBUG_BUFFER_MASK;
  if (n == 0) {
    if (g_debug) debug_print(ANSI_RED, "Error: debug buffer is full!\n");
    debug_file("Error: debug buffer is full!");
    return;
  }
  struct DebugData *out = &g_debug_buffer.debugs[index];
  out->id = g_log_counter;
  out->key_time = 0;
  out->time_offset = 0;
  out->data_type = 2;
  out->len = snprintf(out->data,
                      sizeof(out->data),
                      "%s: %s %s\n",
                      remap_name,
                      key ? key->name : "???",
                      (direction == DOWN) ? "DOWN" : "UP");
  if (out->len >= sizeof(out->data)) {
    out->len = sizeof(out->data) - 1;
  }
  debug_buffer_update_tail(&g_debug_buffer.prod, tail, n);
  if (g_hDebugTimer == NULL) {
    if (!CreateTimerQueueTimer(&g_hDebugTimer, g_hTimerQueue, (WAITORTIMERCALLBACK)debug_callback,
                               &g_debug, DEBUG_START_MS, DEBUG_INTERVAL_MS, 0)) {
      DEBUG(-1, debug_print(ANSI_RED, "CreateTimerQueueTimer failed (%d)\n", GetLastError()));
    }
  }
}

