#ifndef DEBUG_BUFFER_H
#define DEBUG_BUFFER_H

#include <windows.h> // LONG64, InterlockedCompareExchange64
#include <stdint.h>  // uint32_t

#define DEBUG_BUFFER_SIZE 256 // power of 2
#define DEBUG_BUFFER_MASK (DEBUG_BUFFER_SIZE-1)

#ifndef RTE_RING_HTS_HEADTAIL
#define RTE_RING_HTS_HEADTAIL
// From: https://dpdk.readthedocs.io/en/stable/prog_guide/ring_lib.html
union __declspec(align(8)) rte_ring_hts_headtail {
  volatile LONG64 raw;
  struct {
    volatile uint32_t head;
    volatile uint32_t tail;
  } pos;
};
#endif // RTE_RING_HTS_HEADTAIL

struct DebugData {
  int id;
  DWORD key_time;
  int time_offset;
  int data_type;
  int len;
  char data[128];
};

struct DebugBuffer {
  struct DebugData debugs[DEBUG_BUFFER_SIZE];
  volatile union rte_ring_hts_headtail prod;
  volatile union rte_ring_hts_headtail cons;
};

static inline void debug_buffer_init(struct DebugBuffer *debug_buffer) {
  //ZeroMemory(debug_buffer->debugs, sizeof(debug_buffer->debugs));
  debug_buffer->prod.raw = 0;
  debug_buffer->cons.raw = 0;
}

static inline uint32_t debug_buffer_move_prod_head(struct DebugBuffer *debug_buffer, uint32_t *old_head) {
  union rte_ring_hts_headtail new, old;
  do {
    do {
      old.raw = debug_buffer->prod.raw;
    } while (old.pos.head != old.pos.tail);
    if (DEBUG_BUFFER_SIZE - 1 - ((old.pos.head - debug_buffer->cons.pos.tail) & DEBUG_BUFFER_MASK) == 0)
      return 0;
    new.pos.tail = old.pos.tail;
    new.pos.head = old.pos.head + 1;
  } while (InterlockedCompareExchange64(&debug_buffer->prod.raw, new.raw, old.raw) != old.raw);
  *old_head = old.pos.head;
  return 1;
}

static inline uint32_t debug_buffer_move_cons_head(struct DebugBuffer *debug_buffer, int num, uint32_t *old_head) {
  union rte_ring_hts_headtail new, old;
  uint32_t n;
  do {
    do {
      old.raw = debug_buffer->cons.raw;
    } while (old.pos.head != old.pos.tail);
    n = (debug_buffer->prod.pos.tail - old.pos.head) & DEBUG_BUFFER_MASK;
    if (num >= 0 && n > (uint32_t)num) {
      n = (uint32_t)num;
    }
    if (n == 0) return 0;
    new.pos.tail = old.pos.tail;
    new.pos.head = old.pos.head + n;
  } while (InterlockedCompareExchange64(&debug_buffer->cons.raw, new.raw, old.raw) != old.raw);
  *old_head = old.pos.head;
  return n;
}

static inline void debug_buffer_revert_cons_head(struct DebugBuffer *debug_buffer,
                                                 uint32_t new_head) {
  debug_buffer->cons.pos.head = new_head;
}

static inline void debug_buffer_update_tail(volatile union rte_ring_hts_headtail *ht, uint32_t old_tail, uint32_t n) {
  ht->pos.tail = old_tail + n;
}

static inline uint32_t debug_buffer_count(struct DebugBuffer *debug_buffer) {
  return (debug_buffer->prod.pos.tail - debug_buffer->cons.pos.tail) & DEBUG_BUFFER_MASK;
}

static inline uint32_t debug_buffer_free_count(struct DebugBuffer *debug_buffer) {
  return DEBUG_BUFFER_SIZE - 1 - debug_buffer_count(debug_buffer);
}

static inline int debug_buffer_full(struct DebugBuffer *debug_buffer) {
  return debug_buffer_free_count(debug_buffer) == 0;
}

static inline uint32_t debug_buffer_empty(struct DebugBuffer *debug_buffer) {
  return debug_buffer->prod.pos.tail == debug_buffer->cons.pos.tail;
}

static inline int debug_buffer_compare(const struct DebugBuffer *debug_buffer,
                                       uint32_t a, uint32_t b, uint32_t n) {
  for (uint32_t i = 0; i < n; i++) {
    const struct DebugData *da = &debug_buffer->debugs[(a + i) & DEBUG_BUFFER_MASK];
    const struct DebugData *db = &debug_buffer->debugs[(b + i) & DEBUG_BUFFER_MASK];
    if (da->len != db->len ||
        memcmp(da->data, db->data, da->len) != 0)
      return 0;
  }
  return 1; // All equal
}

static inline uint32_t debug_buffer_max_sequence(const struct DebugBuffer *debug_buffer,
                                                 uint32_t start_index,
                                                 uint32_t max_len) {
  const struct DebugData *first = &debug_buffer->debugs[start_index & DEBUG_BUFFER_MASK];
  if (first->time_offset == 0) {
    return 0;
  }
  uint32_t length = 1;
  while (length < max_len) {
    const struct DebugData *entry = &debug_buffer->debugs[(start_index + length) & DEBUG_BUFFER_MASK];
    if (entry->time_offset != 0)
      break;
    length++;
  }
  return length;
}

#endif // DEBUG_BUFFER_H
