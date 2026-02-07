#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include <windows.h> // INPUT, LONG64, InterlockedCompareExchange64, CopyMemory
#include <stdint.h>  // uint32_t

#define INPUT_BUFFER_SIZE 16 // power of 2
#define INPUT_BUFFER_MASK (INPUT_BUFFER_SIZE-1)

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

struct InputBuffer {
  INPUT inputs[INPUT_BUFFER_SIZE + INPUT_BUFFER_SIZE-2];
  volatile union rte_ring_hts_headtail prod;
  volatile union rte_ring_hts_headtail cons;
};

static inline void input_buffer_init(struct InputBuffer *input_buffer) {
  ZeroMemory(input_buffer->inputs, sizeof(input_buffer->inputs));
  input_buffer->prod.raw = 0;
  input_buffer->cons.raw = 0;
}

static inline uint32_t input_buffer_move_prod_head(struct InputBuffer *input_buffer, uint32_t *old_head) {
  union rte_ring_hts_headtail new, old;
  do {
    do {
      old.raw = input_buffer->prod.raw;
    } while (old.pos.head != old.pos.tail);
    if (INPUT_BUFFER_SIZE - 1 - ((old.pos.head - input_buffer->cons.pos.tail) & INPUT_BUFFER_MASK) == 0)
      return 0;
    new.pos.tail = old.pos.tail;
    new.pos.head = old.pos.head + 1;
  } while (InterlockedCompareExchange64(&input_buffer->prod.raw, new.raw, old.raw) != old.raw);
  *old_head = old.pos.head;
  return 1;
}

static inline void input_buffer_clear_cons(struct InputBuffer *input_buffer, uint32_t n) {
  uint32_t tail, ncont;
  tail = input_buffer->cons.pos.tail & INPUT_BUFFER_MASK;
  ncont = INPUT_BUFFER_SIZE - tail;
  if (n > ncont && ncont > 0) {
    ZeroMemory(&input_buffer->inputs[0], (n-ncont)*sizeof(INPUT));
    ZeroMemory(&input_buffer->inputs[tail], ncont*sizeof(INPUT));
  } else {
    ZeroMemory(&input_buffer->inputs[tail], n*sizeof(INPUT));
  }
}

static inline uint32_t input_buffer_move_cons_head(struct InputBuffer *input_buffer, int num, uint32_t *old_head) {
  union rte_ring_hts_headtail new, old;
  uint32_t n, ncont;
  do {
    do {
      old.raw = input_buffer->cons.raw;
    } while (old.pos.head != old.pos.tail);
    n = (input_buffer->prod.pos.tail - old.pos.head) & INPUT_BUFFER_MASK;
    if (num < 0) {
      ncont = (INPUT_BUFFER_SIZE - old.pos.head) & INPUT_BUFFER_MASK;
      if (n > ncont && ncont > 0) {
        if (num < -1) {
          CopyMemory(&input_buffer->inputs[INPUT_BUFFER_SIZE],
                     &input_buffer->inputs[0], (n-ncont)*sizeof(INPUT));
        } else {
          n = ncont;
        }
      }
    } else if (n > (uint32_t)num) {
      n = (uint32_t)num;
    }
    if (n == 0) return 0;
    new.pos.tail = old.pos.tail;
    new.pos.head = old.pos.head + n;
  } while (InterlockedCompareExchange64(&input_buffer->cons.raw, new.raw, old.raw) != old.raw);
  *old_head = old.pos.head;
  return n;
}

static inline void input_buffer_update_tail(volatile union rte_ring_hts_headtail *ht, uint32_t old_tail, uint32_t n) {
  ht->pos.tail = old_tail + n;
}

static inline uint32_t input_buffer_count(struct InputBuffer *input_buffer) {
  return (input_buffer->prod.pos.tail - input_buffer->cons.pos.tail) & INPUT_BUFFER_MASK;
}

static inline uint32_t input_buffer_free_count(struct InputBuffer *input_buffer) {
  return INPUT_BUFFER_SIZE - 1 - input_buffer_count(input_buffer);
}

static inline int input_buffer_full(struct InputBuffer *input_buffer) {
  return input_buffer_free_count(input_buffer) == 0;
}

static inline uint32_t input_buffer_empty(struct InputBuffer *input_buffer) {
  return input_buffer->prod.pos.tail == input_buffer->cons.pos.tail;
}

#endif // INPUT_BUFFER_H
