#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h> // DWORD
#include "keys.h"

// Types
// --------------------------------------
enum State {
  IDLE,
  HELD_DOWN_ALONE,
  HELD_DOWN_WITH_OTHER,
  TAP,
  TAPPED,
  DOUBLE_TAP,
};

struct KeyDefNode {
  const KeyDef *key_def;

  struct KeyDefNode *next;
  struct KeyDefNode *previous;
};

struct LayerNode {
  struct Layer *layer;

  struct LayerNode *next;
};

struct Layer {
  char *name;
  int state;
  int lock, prev_lock;
  struct LayerNode *or_master_layers;
  struct LayerNode *and_master_layers;
  struct LayerNode *and_not_master_layers;
  struct LayerNode *slave_layers;

  struct Layer *next;
};

struct LayerConf {
  struct Layer *layer;
  void (*conf)(struct Layer *layer);

  struct LayerConf *next;
};

struct Remap {
  int id;
  const KeyDef *from;
  struct Layer *layer;
  struct Layer *to_when_press_layer;
  struct Layer *to_when_doublepress_layer;
  struct LayerConf *to_when_tap_lock_layer;
  struct LayerConf *to_when_double_tap_lock_layer;
  struct KeyDefNode *to_when_alone;
  struct KeyDefNode *to_with_other;
  struct KeyDefNode *to_when_doublepress;
  struct KeyDefNode *to_when_tap_lock;
  struct KeyDefNode *to_when_double_tap_lock;
  int to_with_other_dummy;
  int to_when_alone_modifiers;
  int to_with_other_modifiers;
  int to_when_doublepress_modifiers;
  int to_when_tap_lock_modifiers;
  int to_when_double_tap_lock_modifiers;
  int to_when_alone_is_modifier_only;
  int to_when_doublepress_is_modifier_only;

  int tap_lock;
  int double_tap_lock;
  enum State state;
  DWORD time;
  int active_modifiers;

  struct Remap *next;
};

struct RemapNode {
  struct Remap *remap;

  struct RemapNode *next;
};

extern int g_debug;
extern int g_hold_delay;
extern int g_tap_timeout;
extern int g_doublepress_timeout;
extern int g_rehook_timeout;
extern int g_unlock_timeout;
extern int g_scancode;
extern int g_priority;

extern struct Remap *g_remap_list;
extern struct Remap *g_remap_by_id[256];
extern struct RemapNode *g_remap_array[256];
extern struct Layer *g_layer_list;

int load_config_line(char *line, int linenum);
void print_layer_list(struct Layer *head);
int print_status(int back_lines, int force_print);
void free_all();

#endif // CONFIG_H
