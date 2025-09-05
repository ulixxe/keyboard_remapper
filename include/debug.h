#ifndef DEBUG_H
#define DEBUG_H

#include "debug_buffer.h"
#include "keys.h"

//#define DEBUG(cond, x) do { if ((cond & 1 || cond & 0) && g_debug) { x;} } while (0)
#define DEBUG(cond, x) do{}while(0)

static inline const char* ansi_back(int n) {
  static char buf[16];  // plenty for "\x1b[12345F"
  char *p = buf;
  if (n == 0) {
    return "\x1b[G";   // current line
  }
  *p++ = '\x1b';
  *p++ = '[';
  char tmp[12];  // enough for 32-bit int
  int len = 0;
  int num = n;
  if (num == 0) {
    tmp[len++] = '0';
  } else {
    while (num > 0 && len < (int)sizeof(tmp)) {
      tmp[len++] = '0' + (num % 10);
      num /= 10;
    }
  }
  while (len--) {
    *p++ = tmp[len];
  }
  *p++ = 'F';
  *p = '\0';

  return buf;
}

// inline high-resolution timer
static inline double now_sec(void) {
  LARGE_INTEGER freq, counter;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)freq.QuadPart;
}

#define ANSI_BACK(n) ansi_back(n)
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_WHITE   "\x1b[37m"

void debug_file(const char *message);
void debug_print(const char *color, const char *format, ...);
void log_handle_input_start(int scan_code,
                            int virt_code,
                            enum Direction direction,
                            DWORD time,
                            int is_injected,
                            DWORD flags,
                            ULONG_PTR dwExtraInfo);
void log_handle_input_end(int scan_code,
                          int virt_code,
                          enum Direction direction,
                          int block_input);
void log_send_input(char *remap_name,
                    const KeyDef *key,
                    enum Direction direction);

extern HANDLE g_hDebugTimer;
extern struct DebugBuffer g_debug_buffer;
extern int g_debug_buffer_max;
extern int g_log_last_packet_size;

#endif // DEBUG_H
