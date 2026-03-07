#ifndef DEBUG_H
#define DEBUG_H

#include "debug_buffer.h"
#include "keys.h"

//#define DEBUG(cond, x) do { if ((cond & 1 || cond & 0) && g_debug) { x;} } while (0)
#define DEBUG(cond, x) do{}while(0)

struct ProfilerTimer {
  LARGE_INTEGER delta_time, max_delta_time;
};

static inline void profiler_init(struct ProfilerTimer* t) {
  t->delta_time.QuadPart = 0;
  t->max_delta_time.QuadPart = 0;
}

static inline void profiler_start(LARGE_INTEGER* start) {
  QueryPerformanceCounter(start);
}

static inline void profiler_stop(struct ProfilerTimer* t, LARGE_INTEGER start) {
  LARGE_INTEGER stop;
  QueryPerformanceCounter(&stop);
  t->delta_time.QuadPart = stop.QuadPart - start.QuadPart;
  if (t->delta_time.QuadPart > t->max_delta_time.QuadPart)
    t->max_delta_time.QuadPart = t->delta_time.QuadPart;
}

static inline const char* ansi_back(int n) {
  enum {RING = 8, BUFSZ = 16};
  static __declspec(thread) char bufs[RING][BUFSZ];
  static __declspec(thread) unsigned idx;

  char *buf = bufs[idx++ % RING];

  if (n <= 0) {
    buf[0] = '\x1b';
    buf[1] = '[';
    buf[2] = 'G';
    buf[3] = '\0';
    return buf;
  }

  char *p = buf;
  *p++ = '\x1b';
  *p++ = '[';

  // write decimal n
  char tmp[10];
  int len = 0;
  unsigned int num = (unsigned int)n;
  while (num > 0) {
    tmp[len++] = (char)('0' + (num % 10));
    num /= 10;
  }
  while (len--) *p++ = tmp[len];

  *p++ = 'F';
  *p = '\0';
  return buf;
}

#define ANSI_BACK(n)    ansi_back(n)
#define ANSI_ERASE_LINE "\x1b[2K"
#define ANSI_CLEAR_EOL  "\x1b[0K"
#define ANSI_RESET      "\x1b[0m"
#define ANSI_RED        "\x1b[31m"
#define ANSI_GREEN      "\x1b[32m"
#define ANSI_YELLOW     "\x1b[33m"
#define ANSI_BLUE       "\x1b[34m"
#define ANSI_MAGENTA    "\x1b[35m"
#define ANSI_CYAN       "\x1b[36m"
#define ANSI_WHITE      "\x1b[37m"

void debug_file_cleanup(void);
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
extern int g_log_last_packet_size;
extern struct ProfilerTimer g_profiler_timer;

#endif // DEBUG_H
