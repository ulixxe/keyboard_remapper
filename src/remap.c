#include <windows.h>
#include "remap.h"
#include "keyboard_remapper.h"
#include "input_buffer.h"
#include "keys.h"
#include "config.h"
#include "debug.h"

DWORD g_last_input = 0;
int g_filtered_events = 0;
int g_remapped_events = 0;
int g_processed_events = 0;

// Remapping
// -------------------------------------
static int check_layer_state(struct Layer *layer) {
  if (layer->lock) return 1;
  int state = layer->state;
  if (state) {
    struct Remap *remap_iter = g_remap_list;
    while (remap_iter) {
      if ((remap_iter->to_when_press_layer == layer &&
           (remap_iter->state == HELD_DOWN_ALONE ||
            remap_iter->state == HELD_DOWN_WITH_OTHER ||
            remap_iter->state == TAP)) ||
          (remap_iter->to_when_doublepress_layer == layer &&
           remap_iter->state == DOUBLE_TAP)) {
        return 1;
      }
      remap_iter = remap_iter->next;
    }
  }
  struct LayerNode *layer_node_iter = layer->or_master_layers;
  if (layer_node_iter) {
    while (layer_node_iter) {
      if (check_layer_state(layer_node_iter->layer) != 0) {
        return 1;
      }
      layer_node_iter = layer_node_iter->next;
    }
    state = 0;
  }
  layer_node_iter = layer->and_master_layers;
  if (layer_node_iter) {
    while (layer_node_iter) {
      if (check_layer_state(layer_node_iter->layer) == 0) {
        return 0;
      }
      layer_node_iter = layer_node_iter->next;
    }
    state = 1;
  }
  layer_node_iter = layer->and_not_master_layers;
  while (layer_node_iter) {
    if (check_layer_state(layer_node_iter->layer) != 0) {
      return 0;
    }
    layer_node_iter = layer_node_iter->next;
  }
  return state;
}

static void set_layer_state(struct Layer *layer, int state) {
  //if (!layer) return;
  layer->state = state;
  struct LayerNode *slave_iter = layer->slave_layers;
  while (slave_iter) {
    set_layer_state(slave_iter->layer, check_layer_state(slave_iter->layer) ? 1 : slave_iter->layer->lock);
    slave_iter = slave_iter->next;
  }
}

static int check_layer_states(struct LayerNode *layer_list, int expected_state) {
  while (layer_list) {
    if (layer_list->layer->state != expected_state) {
      return 0;
    }
    layer_list = layer_list->next;
  }
  return 1;
}

static int is_master_layer(struct Layer *master_layer, struct Layer *slave_layer) {
  struct LayerNode *master_iter = slave_layer->or_master_layers;
  while (master_iter) {
    if (master_iter->layer == master_layer || is_master_layer(master_layer, master_iter->layer)) {
      return master_iter->layer->state;
    }
    master_iter = master_iter->next;
  }
  if (check_layer_states(slave_layer->and_master_layers, 1) &&
      check_layer_states(slave_layer->and_not_master_layers, 0)) {
    master_iter = slave_layer->and_master_layers;
    while (master_iter) {
      if (master_iter->layer == master_layer || is_master_layer(master_layer, master_iter->layer)) {
        return 1;
      }
      master_iter = master_iter->next;
    }
  }
  return 0;
}

static int has_to_block_modifiers(struct Remap *remap, struct Layer *layer) {
  return remap && remap->layer &&
    (remap->layer == layer || is_master_layer(layer, remap->layer));
}

static int remap_list_depth() {
  int depth = 0;
  struct Remap *remap_iter = g_remap_list;
  while (remap_iter) {
    depth++;
    remap_iter = remap_iter->next;
  }
  return depth;
}

static void append_active_remap(struct Remap **list, struct Remap *elem) {
  while (*list) {
    if (*list == elem) {
      DEBUG(1, debug_print(ANSI_RED, "Remap list depth = %d\n", remap_list_depth()));
      return;
    }
    list = &(*list)->next;
  }
  *list = elem;
  elem->next = NULL;
  DEBUG(1, debug_print(ANSI_RED, "Remap list depth = %d\n", remap_list_depth()));
}

static void remove_active_remap(struct Remap **list, struct Remap *elem) {
  while (*list && *list != elem) {
    list = &(*list)->next;
  }
  if (*list) {
    *list = (*list)->next;
    elem->next = NULL;
  }
  DEBUG(1, debug_print(ANSI_RED, "Remap list depth = %d\n", remap_list_depth()));
}

static int send_key_def_input_down(char *input_name, struct KeyDefNode *head, int remap_id, int modifiers_mask, struct InputBuffer *input_buffer) {
  int key_sent = 0;
  struct KeyDefNode *cur = head;
  do {
    if (!(modifiers_mask & find_modifier_by_virt_code(cur->key_def->virt_code))) {
      if (g_debug) log_send_input(input_name, cur->key_def, DOWN);
      send_input(cur->key_def->scan_code, cur->key_def->virt_code, DOWN, remap_id, input_buffer);
      key_sent = 1;
    }
    cur = cur->next;
  } while (cur != head);
  return key_sent;
}

static int send_key_def_input_up(char *input_name, struct KeyDefNode *head, int remap_id, int modifiers_mask, struct InputBuffer *input_buffer) {
  int key_sent = 0;
  struct KeyDefNode *cur = head;
  do {
    cur = cur->previous;
    if (!(modifiers_mask & find_modifier_by_virt_code(cur->key_def->virt_code))) {
      if (g_debug) log_send_input(input_name, cur->key_def, UP);
      send_input(cur->key_def->scan_code, cur->key_def->virt_code, UP, remap_id, input_buffer);
      key_sent = 1;
    }
  } while (cur != head);
  return key_sent;
}

void unlock_all(struct InputBuffer *input_buffer) {
  struct Layer *layer_iter = g_layer_list;
  while (layer_iter) {
    layer_iter->state = 0;
    layer_iter->lock = 0;
    layer_iter->prev_lock = 0;
    layer_iter = layer_iter->next;
  }
  struct Remap *remap_iter = g_remap_list;
  while (remap_iter) {
    g_remap_list = remap_iter->next;
    if (remap_iter->state == HELD_DOWN_ALONE) {
    } else if (remap_iter->state == HELD_DOWN_WITH_OTHER) {
      if (remap_iter->to_with_other) {
        send_key_def_input_up("unlock_with_other", remap_iter->to_with_other, remap_iter->id, 0, input_buffer);
      }
    } else if (remap_iter->state == TAP) {
      if (remap_iter->to_when_alone) {
        send_key_def_input_up("unlock_when_alone", remap_iter->to_when_alone, remap_iter->id, 0, input_buffer);
      }
    } else if (remap_iter->state == DOUBLE_TAP) {
      if (remap_iter->to_when_doublepress) {
        send_key_def_input_up("unlock_when_doublepress", remap_iter->to_when_doublepress, remap_iter->id, 0, input_buffer);
      }
    }
    if (remap_iter->double_tap_lock) {
      send_key_def_input_up("unlock_when_double_tap_lock", remap_iter->to_when_double_tap_lock, remap_iter->id, 0, input_buffer);
      remap_iter->double_tap_lock = 0;
    }
    if (remap_iter->tap_lock) {
      send_key_def_input_up("unlock_when_tap_lock", remap_iter->to_when_tap_lock, remap_iter->id, 0, input_buffer);
      remap_iter->tap_lock = 0;
    }
    remap_iter->state = IDLE;
    remap_iter->active_modifiers = 0;
    remap_iter->next = NULL;
    remap_iter = g_remap_list;
    DEBUG(1, debug_print(ANSI_RED, "Remap list depth = %d\n", remap_list_depth()));
  }
}

/* @return block_input */
static int event_remapped_key_down(struct Remap *remap, DWORD time, struct InputBuffer *input_buffer) {
  if (remap->state == IDLE) {
    if (remap->to_with_other || remap->to_with_other_dummy) {
      remap->time = time;
      remap->state = HELD_DOWN_ALONE;
    } else {
      remap->time = time;
      remap->state = TAP;
      if (remap->to_when_alone) {
        send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
        remap->active_modifiers = remap->to_when_alone_modifiers;
      }
    }
    if (remap->to_when_press_layer) {
      set_layer_state(remap->to_when_press_layer, 1);
    }
    append_active_remap(&g_remap_list, remap);
  } else if (remap->state == HELD_DOWN_WITH_OTHER) {
    if (remap->to_with_other) {
      send_key_def_input_down("with_other", remap->to_with_other, remap->id, 0, input_buffer);
    }
  } else if (remap->state == TAP) {
    if (remap->to_when_alone) {
      send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
    }
  } else if (remap->state == TAPPED) {
    remap->time = time;
    remap->state = DOUBLE_TAP;
    if (remap->to_when_tap_lock) {
      remap->tap_lock = 1 - remap->tap_lock;
      if (remap->tap_lock == 0) {
        send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
        remap->active_modifiers = 0;
      }
    }
    struct LayerConf *layer_conf = remap->to_when_tap_lock_layer;
    while (layer_conf) {
      layer_conf->layer->lock = layer_conf->layer->prev_lock;
      set_layer_state(layer_conf->layer, layer_conf->layer->lock);
      layer_conf = layer_conf->next;
    }
    if (remap->to_when_doublepress_layer) {
      set_layer_state(remap->to_when_doublepress_layer, 1);
    }
    if (remap->to_when_doublepress) {
      send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
      remap->active_modifiers = remap->to_when_doublepress_modifiers;
    } else if (remap->to_when_doublepress_layer) {
    } else if (remap->to_when_alone) {
      send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
      remap->active_modifiers = remap->to_when_alone_modifiers;
    }
  } else if (remap->state == DOUBLE_TAP) {
    if (remap->to_when_doublepress) {
      send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
    } else if (remap->to_when_doublepress_layer) {
    } else if (remap->to_when_alone) {
      send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
    }
  }
  return 1;
}

/* @return block_input */
static int event_remapped_key_up(struct Remap *remap, DWORD time, struct InputBuffer *input_buffer) {
  if (remap->state == HELD_DOWN_ALONE) {
    if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
      remap->time = time;
      if (g_doublepress_timeout > 0)
        remap->state = TAPPED;
      else
        remap->state = IDLE;
      if (remap->to_when_alone) {
        send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
        send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
      }
      if (remap->to_when_tap_lock) {
        remap->tap_lock = 1 - remap->tap_lock;
        if (remap->tap_lock) {
          send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
          remap->active_modifiers = remap->to_when_tap_lock_modifiers;
        } else {
          send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
          remap->active_modifiers = 0;
        }
      }
      struct LayerConf *layer_conf = remap->to_when_tap_lock_layer;
      while (layer_conf) {
        layer_conf->conf(layer_conf->layer);
        set_layer_state(layer_conf->layer, layer_conf->layer->lock);
        layer_conf = layer_conf->next;
      }
    } else {
      remap->state = IDLE;
    }
    if (remap->to_when_press_layer) {
      set_layer_state(remap->to_when_press_layer, remap->to_when_press_layer->lock);
    }
  } else if (remap->state == HELD_DOWN_WITH_OTHER) {
    remap->state = IDLE;
    if (remap->to_with_other) {
      send_key_def_input_up("with_other", remap->to_with_other, remap->id, 0, input_buffer);
      remap->active_modifiers = 0;
    }
    if (remap->to_when_press_layer) {
      set_layer_state(remap->to_when_press_layer, remap->to_when_press_layer->lock);
    }
  } else if (remap->state == TAP) {
    if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
      remap->time = time;
      if (g_doublepress_timeout > 0)
        remap->state = TAPPED;
      else
        remap->state = IDLE;
      if (remap->to_when_alone) {
        send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
        remap->active_modifiers = 0;
      }
      if (remap->to_when_tap_lock) {
        remap->tap_lock = 1 - remap->tap_lock;
        if (remap->tap_lock) {
          send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
          remap->active_modifiers = remap->to_when_tap_lock_modifiers;
        } else {
          send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
          remap->active_modifiers = 0;
        }
      }
      struct LayerConf *layer_conf = remap->to_when_tap_lock_layer;
      while (layer_conf) {
        layer_conf->conf(layer_conf->layer);
        set_layer_state(layer_conf->layer, layer_conf->layer->lock);
        layer_conf = layer_conf->next;
      }
    } else {
      remap->state = IDLE;
      if (remap->to_when_alone) {
        send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
        remap->active_modifiers = 0;
      }
    }
    if (remap->to_when_press_layer) {
      set_layer_state(remap->to_when_press_layer, remap->to_when_press_layer->lock);
    }
  } else if (remap->state == DOUBLE_TAP) {
    remap->state = IDLE;
    if (remap->to_when_doublepress) {
      send_key_def_input_up("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
      remap->active_modifiers = 0;
    } else if (remap->to_when_doublepress_layer) {
    } else if (remap->to_when_alone) {
      send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
      remap->active_modifiers = 0;
    }
    if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
      if (remap->to_when_double_tap_lock) {
        remap->double_tap_lock = 1 - remap->double_tap_lock;
        if (remap->double_tap_lock) {
          send_key_def_input_down("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, 0, input_buffer);
          remap->active_modifiers = remap->to_when_double_tap_lock_modifiers;
        } else {
          send_key_def_input_up("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, 0, input_buffer);
          remap->active_modifiers = 0;
        }
      }
      struct LayerConf *layer_conf = remap->to_when_double_tap_lock_layer;
      while (layer_conf) {
        layer_conf->conf(layer_conf->layer);
        set_layer_state(layer_conf->layer, layer_conf->layer->lock);
        layer_conf = layer_conf->next;
      }
    }
    if (remap->to_when_doublepress_layer) {
      set_layer_state(remap->to_when_doublepress_layer, remap->to_when_doublepress_layer->lock);
    }
  }
  if (remap->state == IDLE && remap->tap_lock == 0 && remap->double_tap_lock == 0) {
    remove_active_remap(&g_remap_list, remap);
  }
  return 1;
}

/* @return block_input */
static int event_other_input(int virt_code, enum Direction direction, DWORD time, int remap_id, struct InputBuffer *input_buffer) {
  int block_input = 0;
  if (direction == DOWN && !find_modifier_by_virt_code(virt_code)) {
    struct Remap *remap = g_remap_list;
    while (remap) {
      if (remap->id != remap_id) {
        if (remap->state == HELD_DOWN_ALONE) {
          if ((g_hold_delay > 0) && (time - remap->time < g_hold_delay) && remap->to_when_alone) {
            remap->state = TAP;
            block_input |= send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
            remap->active_modifiers = remap->to_when_alone_modifiers;
          } else {
            if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_press_layer)) {
              remap->state = HELD_DOWN_WITH_OTHER;
              if (remap->to_with_other) {
                block_input |= send_key_def_input_down("with_other", remap->to_with_other, remap->id, 0, input_buffer);
                remap->active_modifiers = remap->to_with_other_modifiers;
              }
            }
          }
        } else if (remap->state == HELD_DOWN_WITH_OTHER) {
          if (remap->to_with_other) {
            if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_press_layer)) {
              block_input |= send_key_def_input_down("with_other", remap->to_with_other, remap->id, 0, input_buffer);
            } else {
              block_input |= send_key_def_input_up("with_other", remap->to_with_other, remap->id, g_remap_by_id[remap_id]->active_modifiers, input_buffer);
            }
          }
        } else if (remap->state == TAP) {
          if (remap->to_when_alone && remap->to_when_alone_is_modifier_only) {
            if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_press_layer)) {
              block_input |= send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
            } else {
              block_input |= send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, g_remap_by_id[remap_id]->active_modifiers, input_buffer);
            }
          }
        } else if (remap->state == DOUBLE_TAP) {
          if (remap->to_when_doublepress && remap->to_when_doublepress_is_modifier_only) {
            if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_doublepress_layer)) {
              block_input |= send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
            } else {
              block_input |= send_key_def_input_up("when_doublepress", remap->to_when_doublepress, remap->id, g_remap_by_id[remap_id]->active_modifiers, input_buffer);
            }
          }
        } else {
          if (remap->double_tap_lock) {
            block_input |= send_key_def_input_down("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, 0, input_buffer);
          }
          if (remap->tap_lock) {
            block_input |= send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
          }
        }
        remap->time = 0; // disable tap and double_tap
      }
      remap = remap->next;
    }
  }
  return -block_input;
}

/* @return block_input */
int handle_input(int scan_code, int virt_code, enum Direction direction, DWORD time, int is_injected, DWORD flags, ULONG_PTR dwExtraInfo, struct InputBuffer *input_buffer) {
  struct Remap *remap_for_input;
  int block_input;
  int remap_id = 0; // if 0 then no remapped injected key

  if (g_debug) log_handle_input_start(scan_code, virt_code, direction, time, is_injected, flags, dwExtraInfo);
  if ((g_unlock_timeout > 0) && (time - g_last_input > g_unlock_timeout)) {
    unlock_all(input_buffer);
  }
  if (is_injected && ((dwExtraInfo & 0xFFFFFF00) != INJECTED_KEY_ID || dwExtraInfo == INJECTED_KEY_ID)) {
    // Note: passthrough of injected keys from other tools or
    //   from Dual-key-remap self when passthrough is requested (remap_id = 0).
    block_input = 0;
    if ((g_rehook_timeout > 0) && (time - g_last_input > g_rehook_timeout)) {
      rehook();
      g_last_input = time;
    }
  } else if (scan_code == 0x022A) {
    // To filter out unwanted key events generated on certain HP laptops
    block_input = 1;
    g_filtered_events++;
  } else {
    g_last_input = time;
    if (is_injected) {
      // Note: injected keys are never remapped to avoid complex nested scenarios
      remap_for_input = NULL;
      remap_id = dwExtraInfo & 0x000000FF;
    } else {
      struct Remap **list = &g_remap_list;
      while (*list) {
        if ((*list)->state == TAPPED && (time - (*list)->time >= g_doublepress_timeout)) {
          (*list)->state = IDLE;
          if ((*list)->tap_lock == 0 && (*list)->double_tap_lock == 0) {
            struct Remap *elem = *list;
            *list = (*list)->next;
            elem->next = NULL;
            DEBUG(1, debug_print(ANSI_RED, "Remap list depth = %d\n", remap_list_depth()));
            continue;
          }
        } else if ((*list)->from->virt_code == virt_code) {
          break;
        }
        list = &(*list)->next;
      }
      remap_for_input = *list;
      if (remap_for_input == NULL) {
        struct RemapNode *remap_node_iter = g_remap_array[virt_code & 0xFF];
        while (remap_node_iter) {
          if (remap_node_iter->remap->layer == NULL) {
            break;
          } else if (remap_node_iter->remap->layer->state) {
            break;
          }
          remap_node_iter = remap_node_iter->next;
        }
        if (remap_node_iter) {
          remap_for_input = remap_node_iter->remap;
        } else {
          // TODO: auto_unlock if not modifier
        }
      }
    }
    if (remap_for_input) {
      if (direction == UP) {
        block_input = event_remapped_key_up(remap_for_input, time, input_buffer);
      } else {
        block_input = event_remapped_key_down(remap_for_input, time, input_buffer);
      }
      g_remapped_events++;
    } else {
      block_input = event_other_input(virt_code, direction, time, remap_id, input_buffer);
      g_processed_events++;
    }
  }
  if (g_debug) log_handle_input_end(scan_code, virt_code, direction, block_input);
  return block_input;
}
