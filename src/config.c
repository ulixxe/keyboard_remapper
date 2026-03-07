#include <windows.h>
#include <stdio.h>
#include "keys.h"
#include "config.h"
#include "input_buffer.h"
#include "keyboard_remapper.h"
#include "debug_buffer.h"
#include "debug.h"

// Globals
int g_debug = 0;
int g_hold_delay = 0;
int g_tap_timeout = 0;
int g_doublepress_timeout = 0;
int g_rehook_timeout = 1000;
int g_unlock_timeout = 60000;
int g_scancode = 0;
int g_priority = 1;
struct Remap *g_remap_list = NULL;
struct Remap *g_remap_by_id[REMAP_ID_SIZE] = {NULL};
struct RemapNode *g_remap_array[VIRT_CODE_SIZE] = {NULL};
struct Layer *g_layer_list = NULL;
static struct Remap *g_remap_parsee = NULL;
static struct Layer *g_layer_parsee = NULL;

void print_layer_list(struct Layer *head) {
  struct Layer *current = head;
  while (current) {
    printf("Layer Name: %s\n", current->name);
    printf("Master Layers:\n");
    struct LayerNode *or_master_iter = current->or_master_layers;
    while (or_master_iter) {
      printf("  - OR  %s\n", or_master_iter->layer->name);
      or_master_iter = or_master_iter->next;
    }
    struct LayerNode *and_master_iter = current->and_master_layers;
    while (and_master_iter) {
      printf("  - AND %s\n", and_master_iter->layer->name);
      and_master_iter = and_master_iter->next;
    }
    struct LayerNode *and_not_master_iter = current->and_not_master_layers;
    while (and_not_master_iter) {
      printf("  - NOT %s\n", and_not_master_iter->layer->name);
      and_not_master_iter = and_not_master_iter->next;
    }
    printf("Slave Layers:\n");
    struct LayerNode *slave_iter = current->slave_layers;
    while (slave_iter) {
      printf("  -     %s\n", slave_iter->layer->name);
      slave_iter = slave_iter->next;
    }
    printf("\n");
    current = current->next;
  }
}

int print_status(int back_lines, int force_print) {
  static struct Status prev_status = {0};
  static int prev_input_buffer_count = 0;
  static int prev_debug_buffer_count = 0;
  static LARGE_INTEGER prev_delta_time = {0};
  int curr_input_buffer_count = 0;
  int curr_debug_buffer_count = 0;
  int lines = 1+7+1+1+1+1;
  if (force_print != 0)
    printf("%s\n", ANSI_CLEAR_EOL);
  else if (back_lines != 0)
    back_lines--;
  else
    printf("\n");
  if (force_print != 0 || g_status.keyboard_blocked_events != prev_status.keyboard_blocked_events || g_status.keyboard_passthrough_events != prev_status.keyboard_passthrough_events) {
    if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
    printf("Keyboard blocked events:     %6d/%6d%s\n", g_status.keyboard_blocked_events, g_status.keyboard_blocked_events+g_status.keyboard_passthrough_events, ANSI_CLEAR_EOL);
    printf("Keyboard passthrough events: %6d/%6d%s\n", g_status.keyboard_passthrough_events, g_status.keyboard_blocked_events+g_status.keyboard_passthrough_events, ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 2;
  else
    printf("\n\n");
  if (force_print != 0 || g_status.mouse_blocked_events != prev_status.mouse_blocked_events || g_status.mouse_passthrough_events != prev_status.mouse_passthrough_events) {
    if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
    printf("Mouse blocked events:        %6d/%6d%s\n", g_status.mouse_blocked_events, g_status.mouse_blocked_events+g_status.mouse_passthrough_events, ANSI_CLEAR_EOL);
    printf("Mouse passthrough events:    %6d/%6d%s\n", g_status.mouse_passthrough_events, g_status.mouse_blocked_events+g_status.mouse_passthrough_events, ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 2;
  else
    printf("\n\n");
  if (force_print != 0 || g_status.remapped_events != prev_status.remapped_events || g_status.processed_events != prev_status.processed_events || g_status.filtered_events != prev_status.filtered_events) {
    if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
    printf("Remapped events:             %6d/%6d%s\n", g_status.remapped_events, g_status.remapped_events+g_status.processed_events+g_status.filtered_events, ANSI_CLEAR_EOL);
    printf("Processed events:            %6d/%6d%s\n", g_status.processed_events, g_status.remapped_events+g_status.processed_events+g_status.filtered_events, ANSI_CLEAR_EOL);
    printf("Filtered out events:         %6d/%6d%s\n", g_status.filtered_events, g_status.remapped_events+g_status.processed_events+g_status.filtered_events, ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 3;
  else
    printf("\n\n\n");
  struct Layer *layer_iter = g_layer_list;
  if (force_print != 0) {
    if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
    printf("Active Layers:%s\n", ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 1;
  else
    printf("\n");
  while (layer_iter) {
    if (layer_iter->state) {
      if (force_print != 0 || 1) {
        if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
        printf("  - %s%s\n", layer_iter->name, ANSI_CLEAR_EOL);
      } else if (back_lines != 0)
        back_lines = back_lines - 1;
      else
        printf("\n");
      lines++;
    }
    layer_iter = layer_iter->next;
  }
  if (force_print != 0) {
    if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
    printf("Active Remappings:%s\n", ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 1;
  else
    printf("\n");
  struct Remap *remap_iter = g_remap_list;
  while (remap_iter) {
    if (force_print != 0 || 1) {
      if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
      printf("  - %s", remap_iter->from->name);
      switch (remap_iter->state) {
      case IDLE:
        printf(" (%s)", "IDLE");
        break;
      case HELD_DOWN_ALONE:
        printf(" (%s)", "HELD_DOWN_ALONE");
        break;
      case HELD_DOWN_WITH_OTHER:
        printf(" (%s)", "HELD_DOWN_WITH_OTHER");
        break;
      case TAP:
        printf(" (%s)", "TAP");
        break;
      case TAPPED:
        printf(" (%s)", "TAPPED");
        break;
      case DOUBLE_TAP:
        printf(" (%s)", "DOUBLE_TAP");
        break;
      default:
        printf(" (%s)", "UNKNOWN");
        break;
      }
      printf("%s\n", ANSI_CLEAR_EOL);
    } else if (back_lines != 0)
      back_lines = back_lines - 1;
    else
      printf("\n");
    remap_iter = remap_iter->next;
    lines++;
  }
  curr_input_buffer_count = input_buffer_count(&g_input_buffer);
  if (force_print != 0 || lines != g_status_lines ||
      curr_input_buffer_count != prev_input_buffer_count || g_status.input_buffer_max != prev_status.input_buffer_max) {
    if (back_lines != 0) {printf(ANSI_BACK(back_lines)); back_lines = 0;}
    printf("Input buffer utilization: %3d/%3d (%3d/%3d peak)%s\n",
           curr_input_buffer_count,
           INPUT_BUFFER_SIZE,
           g_status.input_buffer_max,
           INPUT_BUFFER_SIZE,
           ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 1;
  else
    printf("\n");
  curr_debug_buffer_count = debug_buffer_count(&g_debug_buffer);
  if (force_print != 0 ||  lines != g_status_lines ||
      curr_debug_buffer_count != prev_debug_buffer_count || g_status.debug_buffer_max != prev_status.debug_buffer_max) {
    printf("Debug buffer utilization: %3d/%3d (%3d/%3d peak)%s\n",
           curr_debug_buffer_count,
           DEBUG_BUFFER_SIZE,
           g_status.debug_buffer_max,
           DEBUG_BUFFER_SIZE,
           ANSI_CLEAR_EOL);
  } else if (back_lines != 0)
    back_lines = back_lines - 1;
  else
    printf("\n");
  if (force_print != 0 ||  lines != g_status_lines ||
      g_profiler_timer.delta_time.QuadPart != prev_delta_time.QuadPart) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    printf("Response time: %.3f us (max=%.3f us) @ %.0f MHz%s",
           1e6 * (double)g_profiler_timer.delta_time.QuadPart / (double)freq.QuadPart,
           1e6 * (double)g_profiler_timer.max_delta_time.QuadPart / (double)freq.QuadPart,
           1e-6 * (double)freq.QuadPart,
           ANSI_CLEAR_EOL);
  }
  prev_status = g_status;
  prev_input_buffer_count = curr_input_buffer_count;
  prev_debug_buffer_count = curr_debug_buffer_count;
  prev_delta_time = g_profiler_timer.delta_time;
  return lines;
}

static void toggle_layer_lock(struct Layer *layer) {
  layer->prev_lock = layer->lock;
  layer->lock = 1 - layer->lock;
}

static void set_layer_lock(struct Layer *layer) {
  layer->prev_lock = layer->lock;
  layer->lock = 1;
}

static void reset_layer_lock(struct Layer *layer) {
  layer->prev_lock = layer->lock;
  layer->lock = 0;
}

static struct KeyDefNode *new_key_node(const KeyDef *key_def) {
  struct KeyDefNode *key_node = malloc(sizeof(struct KeyDefNode));
  key_node->key_def = key_def;
  key_node->next = key_node;
  key_node->previous = key_node;
  return key_node;
}

static struct LayerNode *new_layer_node(struct Layer *layer) {
  struct LayerNode *layer_node = malloc(sizeof(struct LayerNode));
  layer_node->layer = layer;
  layer_node->next = NULL;
  return layer_node;
}

static struct Layer *new_layer(char *name) {
  struct Layer *layer = malloc(sizeof(struct Layer));
  layer->name = strdup(name);
  layer->state = 0;
  layer->lock = 0;
  layer->prev_lock = 0;
  layer->or_master_layers = NULL;
  layer->and_master_layers = NULL;
  layer->and_not_master_layers = NULL;
  layer->slave_layers = NULL;
  layer->next = NULL;
  return layer;
}

static struct LayerConf *new_layer_conf(struct Layer *layer, void (*conf)(struct Layer *layer)) {
  struct LayerConf *layer_conf = malloc(sizeof(struct LayerConf));
  layer_conf->layer = layer;
  layer_conf->conf = conf;
  layer_conf->next = NULL;
  return layer_conf;
}

static struct Remap *new_remap(const KeyDef *from,
                               struct Layer *layer,
                               struct KeyDefNode *to_when_alone,
                               struct KeyDefNode *to_with_other,
                               struct KeyDefNode *to_when_doublepress,
                               struct KeyDefNode *to_when_tap_lock,
                               struct KeyDefNode *to_when_double_tap_lock) {
  struct Remap *remap = malloc(sizeof(struct Remap));
  remap->id = 0;
  remap->from = from;
  remap->layer = layer;
  remap->to_when_press_layer = NULL;
  remap->to_when_doublepress_layer = NULL;
  remap->to_when_tap_lock_layer = NULL;
  remap->to_when_double_tap_lock_layer = NULL;
  remap->to_when_alone = to_when_alone;
  remap->to_with_other = to_with_other;
  remap->to_when_doublepress = to_when_doublepress;
  remap->to_when_tap_lock = to_when_tap_lock;
  remap->to_when_double_tap_lock = to_when_double_tap_lock;
  remap->to_with_other_dummy = 0;
  remap->to_when_alone_modifiers = 0;
  remap->to_with_other_modifiers = 0;
  remap->to_when_doublepress_modifiers = 0;
  remap->to_when_tap_lock_modifiers = 0;
  remap->to_when_double_tap_lock_modifiers = 0;
  remap->to_when_alone_is_modifier_only = 0;
  remap->to_when_doublepress_is_modifier_only = 0;
  remap->tap_lock = 0;
  remap->double_tap_lock = 0;
  remap->state = IDLE;
  remap->time = 0;
  remap->active_modifiers = 0;
  remap->next = NULL;
  return remap;
}

static struct RemapNode *new_remap_node(struct Remap *remap) {
  struct RemapNode *remap_node = malloc(sizeof(struct RemapNode));
  remap_node->remap = remap;
  remap_node->next = NULL;
  return remap_node;
}

static void append_key_node(struct KeyDefNode *head, const KeyDef *key_def) {
  head->previous->next = new_key_node(key_def);
  head->previous->next->previous = head->previous;
  head->previous = head->previous->next;
  head->previous->next = head;
}

static void append_layer_node(struct LayerNode **list, struct LayerNode *elem) {
  while (*list) list = &(*list)->next;
  *list = elem;
}

static int key_eq(struct KeyDefNode *head_a, struct KeyDefNode *head_b) {
  struct KeyDefNode *cur_a = head_a;
  struct KeyDefNode *cur_b = head_b;
  while (cur_a && cur_b) {
    if (cur_a->key_def != cur_b->key_def) return 0;
    cur_a = cur_a->next;
    cur_b = cur_b->next;
    if (cur_a == head_a && cur_b == head_b) return 1;
  }
  return 0;
}

static int modifiers(struct KeyDefNode *head) {
  struct KeyDefNode *cur = head;
  int modifiers = 0;
  do {
    modifiers |= cur->key_def->modifier;
    cur = cur->next;
  } while (cur != head);
  return modifiers;
}

static int is_modifier_only(struct KeyDefNode *head) {
  struct KeyDefNode *cur = head;
  int modifier_only = 1;
  do {
    modifier_only *= cur->key_def->modifier;
    cur = cur->next;
  } while(cur != head);
  return modifier_only ? 1 : 0;
}

static void free_key_nodes(struct KeyDefNode *head) {
  if (head) {
    head->previous->next = NULL;
    struct KeyDefNode *cur = head;
    while (cur) {
      struct KeyDefNode *key_node = cur;
      cur = cur->next;
      free(key_node);
    }
  }
}

static void free_layer_nodes(struct LayerNode *head) {
  struct LayerNode *cur = head;
  while (cur) {
    struct LayerNode *layer_node = cur;
    cur = cur->next;
    free(layer_node);
  }
}

static void free_layers(struct Layer *head) {
  struct Layer *cur = head;
  while (cur) {
    struct Layer *layer = cur;
    cur = cur->next;
    free(layer->name);
    free_layer_nodes(layer->or_master_layers);
    free_layer_nodes(layer->and_master_layers);
    free_layer_nodes(layer->and_not_master_layers);
    free_layer_nodes(layer->slave_layers);
    free(layer);
  }
}

static void free_layer_confs(struct LayerConf *head) {
  struct LayerConf *cur = head;
  while (cur) {
    struct LayerConf *layer_conf = cur;
    cur = cur->next;
    free(layer_conf);
  }
}

static void free_remap(struct Remap *remap) {
  free_layer_confs(remap->to_when_tap_lock_layer);
  free_layer_confs(remap->to_when_double_tap_lock_layer);
  free_key_nodes(remap->to_when_alone);
  free_key_nodes(remap->to_with_other);
  free_key_nodes(remap->to_when_doublepress);
  free_key_nodes(remap->to_when_tap_lock);
  free_key_nodes(remap->to_when_double_tap_lock);
  free(remap);
}

static void free_remap_nodes(struct RemapNode *head) {
  struct RemapNode *cur = head;
  while (cur) {
    struct RemapNode *remap_node = cur;
    cur = cur->next;
    free_remap(remap_node->remap);
    free(remap_node);
  }
}

void free_all() {
  free(g_remap_parsee);
  g_remap_parsee = NULL;
  g_layer_parsee = NULL;
  g_remap_list = NULL;
  free_layers(g_layer_list);
  g_layer_list = NULL;
  for (int i = 0; i < VIRT_CODE_SIZE; i++) {
    free_remap_nodes(g_remap_array[i]);
    g_remap_array[i] = NULL;
  }
  for (int i = 0; i < REMAP_ID_SIZE; i++)
    g_remap_by_id[i] = NULL;
}

static struct Layer *find_layer(struct Layer *list, char *name) {
  while (list) {
    if (strcmp(list->name, name) == 0) {
      return list;
    }
    list = list->next;
  }
  return NULL;
}

static struct Layer *append_layer(struct Layer **list, struct Layer *elem) {
  while (*list) list = &(*list)->next;
  *list = elem;
  return *list;
}

static void append_layer_conf(struct LayerConf **list, struct LayerConf *elem) {
  while (*list) list = &(*list)->next;
  *list = elem;
}

static int register_remap(struct Remap *remap) {
  if (g_remap_list) {
    struct Remap *tail = g_remap_list;
    while (tail->next) tail = tail->next;
    if (tail->id >= REMAP_ID_SIZE-1) return 1;
    tail->next = remap;
    remap->id = tail->id + 1;
  } else {
    g_remap_list = remap;
    remap->id = 1;
  }
  g_remap_by_id[remap->id] = remap;
  if (key_eq(remap->to_when_alone, remap->to_with_other)) {
    free_key_nodes(remap->to_with_other);
    remap->to_with_other = NULL;
  }
  if (key_eq(remap->to_when_alone, remap->to_when_doublepress)) {
    free_key_nodes(remap->to_when_doublepress);
    remap->to_when_doublepress = NULL;
  }
  if (remap->to_when_alone) {
    remap->to_when_alone_modifiers = modifiers(remap->to_when_alone);
    remap->to_when_alone_is_modifier_only = is_modifier_only(remap->to_when_alone);
  }
  if (remap->to_with_other) {
    remap->to_with_other_modifiers = modifiers(remap->to_with_other);
    if (!is_modifier_only(remap->to_with_other)) {
      free_key_nodes(remap->to_with_other);
      remap->to_with_other = NULL;
      remap->to_with_other_modifiers = 0;
    }
  }
  if (remap->to_when_doublepress) {
    remap->to_when_doublepress_modifiers = modifiers(remap->to_when_doublepress);
    remap->to_when_doublepress_is_modifier_only = is_modifier_only(remap->to_when_doublepress);
  }
  if (remap->to_when_tap_lock) {
    remap->to_when_tap_lock_modifiers = modifiers(remap->to_when_tap_lock);
  }
  if (remap->to_when_double_tap_lock) {
    remap->to_when_double_tap_lock_modifiers = modifiers(remap->to_when_double_tap_lock);
  }
  return 0;
}

// Config
static void trim_newline(char *str) {
  str[strcspn(str, "\r\n")] = 0;
  int length = strlen(str);
  while (length > 0 && isspace((unsigned char)str[length - 1])) {
    str[length - 1] = 0;
    length--;
  }
}

static int parsee_is_valid() {
  return g_remap_parsee &&
    g_remap_parsee->from &&
    (g_remap_parsee->to_when_alone || g_remap_parsee->to_with_other ||
     g_remap_parsee->to_when_doublepress ||
     g_remap_parsee->to_when_tap_lock || g_remap_parsee->to_when_double_tap_lock ||
     g_remap_parsee->to_when_press_layer || g_remap_parsee->to_when_doublepress_layer ||
     g_remap_parsee->to_when_tap_lock_layer || g_remap_parsee->to_when_double_tap_lock_layer);
}

/* @return error */
int load_config_line(char *line, int linenum) {
  if (line == NULL) {
    if (parsee_is_valid()) {
      if (register_remap(g_remap_parsee)) {
        g_remap_parsee = NULL;
        printf("Config error (line %d): Exceeded the maximum limit of %d remappings.\n", linenum, REMAP_ID_SIZE-1);
        return 1;
      }
      g_remap_parsee = NULL;
    }
    while (g_remap_list) {
      struct RemapNode *remap_node = new_remap_node(g_remap_list);
      int index = g_remap_list->from->virt_code & VIRT_CODE_MASK;

      if (g_remap_list->layer || (g_remap_array[index] && !g_remap_array[index]->remap->layer)) {
        remap_node->next = g_remap_array[index];
        g_remap_array[index] = remap_node;
      } else {
        if (g_remap_array[index]) {
          struct RemapNode *tail = g_remap_array[index];
          while (tail->next && tail->next->remap->layer) tail = tail->next;
          remap_node->next = tail->next;
          tail->next = remap_node;
        } else {
          g_remap_array[index] = remap_node;
        }
      }
      g_remap_list = g_remap_list->next;
    }
    return 0;
  }

  trim_newline(line);

  // Ignore comments and empty lines
  if (line[0] == '#' || line[0] == '\0') {
    return 0;
  }

  // Handle config declaration
  if (sscanf(line, "debug=%d", &g_debug)) {
    if (g_debug == 1 || g_debug == 0)
      return 0;
  }

  if (sscanf(line, "hold_delay=%d", &g_hold_delay)) {
    return 0;
  }

  if (sscanf(line, "tap_timeout=%d", &g_tap_timeout)) {
    return 0;
  }

  if (sscanf(line, "doublepress_timeout=%d", &g_doublepress_timeout)) {
    return 0;
  }

  if (sscanf(line, "rehook_timeout=%d", &g_rehook_timeout)) {
    return 0;
  }

  if (sscanf(line, "unlock_timeout=%d", &g_unlock_timeout)) {
    return 0;
  }

  if (sscanf(line, "scancode=%d", &g_scancode)) {
    if (g_scancode == 1 || g_scancode == 0)
      return 0;
  }

  if (sscanf(line, "priority=%d", &g_priority)) {
    if (g_priority == 1 || g_priority == 0)
      return 0;
  }

  // Handle key remappings
  char *after_eq = (char *)strchr(line, '=');
  if (!after_eq) {
    printf("Config error (line %d): Couldn't understand '%s'.\n", linenum, line);
    return 1;
  }
  char *key_name = after_eq + 1;
  const KeyDef *key_def = find_key_def_by_name(key_name);
  if (!key_def && strlen(key_name) != 0 &&
      strncmp(key_name, "layer", strlen("layer")) &&
      strncmp(key_name, "toggle_layer", strlen("toggle_layer")) &&
      strncmp(key_name, "set_layer", strlen("set_layer")) &&
      strncmp(key_name, "reset_layer", strlen("reset_layer"))) {
    printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
    return 1;
  }

  if (g_remap_parsee == NULL) {
    g_remap_parsee = new_remap(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  }

  if (strncmp(line, "remap_key=", strlen("remap_key=")) == 0) {
    if (g_remap_parsee->from && !parsee_is_valid()) {
      printf("Config error (line %d): Incomplete remapping.\n"
             "Each remapping must have a 'remap_key', 'when_alone', and 'with_other'.\n",
             linenum);
      return 1;
    }
    if (g_remap_parsee->from && parsee_is_valid()) {
      if (register_remap(g_remap_parsee)) {
        g_remap_parsee = NULL;
        printf("Config error (line %d): Exceeded the maximum limit of %d remappings.\n", linenum, REMAP_ID_SIZE-1);
        return 1;
      }
      g_remap_parsee = new_remap(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    g_remap_parsee->from = key_def;
  } else if (strncmp(line, "layer=", strlen("layer=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      g_remap_parsee->layer = find_layer(g_layer_list, key_name);
      if (g_remap_parsee->layer == NULL) {
        g_remap_parsee->layer = append_layer(&g_layer_list, new_layer(key_name));
      }
    } else {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
  } else if (strncmp(line, "when_alone=", strlen("when_alone=")) == 0) {
    if (!key_def) {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
    if (g_remap_parsee->to_when_alone == NULL) {
      g_remap_parsee->to_when_alone = new_key_node(key_def);
    } else {
      append_key_node(g_remap_parsee->to_when_alone, key_def);
    }
  } else if (strncmp(line, "with_other=", strlen("with_other=")) == 0) {
    if (!key_def && strlen(key_name) != 0) {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
    if (strlen(key_name) == 0) {
      g_remap_parsee->to_with_other_dummy = 1;
    } else if (g_remap_parsee->to_with_other == NULL) {
      g_remap_parsee->to_with_other = new_key_node(key_def);
    } else {
      append_key_node(g_remap_parsee->to_with_other, key_def);
    }
  } else if (strncmp(line, "when_press=", strlen("when_press=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      g_remap_parsee->to_when_press_layer = find_layer(g_layer_list, key_name);
      if (g_remap_parsee->to_when_press_layer == NULL) {
        g_remap_parsee->to_when_press_layer = append_layer(&g_layer_list, new_layer(key_name));
      }
    } else {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
  } else if (strncmp(line, "when_doublepress=", strlen("when_doublepress=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      g_remap_parsee->to_when_doublepress_layer = find_layer(g_layer_list, key_name);
      if (g_remap_parsee->to_when_doublepress_layer == NULL) {
        g_remap_parsee->to_when_doublepress_layer = append_layer(&g_layer_list, new_layer(key_name));
      }
    } else {
      if (!key_def) {
        printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
        return 1;
      }
      if (g_remap_parsee->to_when_doublepress == NULL) {
        g_remap_parsee->to_when_doublepress = new_key_node(key_def);
      } else {
        append_key_node(g_remap_parsee->to_when_doublepress, key_def);
      }
    }
  } else if (strncmp(line, "when_tap_lock=", strlen("when_tap_lock=")) == 0) {
    if (strncmp(key_name, "toggle_layer", strlen("toggle_layer")) == 0) {
      struct Layer *layer = find_layer(g_layer_list, key_name + strlen("toggle_"));
      if (layer == NULL) {
        layer = append_layer(&g_layer_list, new_layer(key_name + strlen("toggle_")));
      }
      append_layer_conf(&g_remap_parsee->to_when_tap_lock_layer, new_layer_conf(layer, toggle_layer_lock));
    } else if (strncmp(key_name, "set_layer", strlen("set_layer")) == 0) {
      struct Layer *layer = find_layer(g_layer_list, key_name + strlen("set_"));
      if (layer == NULL) {
        layer = append_layer(&g_layer_list, new_layer(key_name + strlen("set_")));
      }
      append_layer_conf(&g_remap_parsee->to_when_tap_lock_layer, new_layer_conf(layer, set_layer_lock));
    } else if (strncmp(key_name, "reset_layer", strlen("reset_layer")) == 0) {
      struct Layer *layer = find_layer(g_layer_list, key_name + strlen("reset_"));
      if (layer == NULL) {
        layer = append_layer(&g_layer_list, new_layer(key_name + strlen("reset_")));
      }
      append_layer_conf(&g_remap_parsee->to_when_tap_lock_layer, new_layer_conf(layer, reset_layer_lock));
    } else {
      if (!key_def) {
        printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
        return 1;
      }
      if (g_remap_parsee->to_when_tap_lock == NULL) {
        g_remap_parsee->to_when_tap_lock = new_key_node(key_def);
      } else {
        append_key_node(g_remap_parsee->to_when_tap_lock, key_def);
      }
    }
  } else if (strncmp(line, "when_double_tap_lock=", strlen("when_double_tap_lock=")) == 0) {
    if (strncmp(key_name, "toggle_layer", strlen("toggle_layer")) == 0) {
      struct Layer *layer = find_layer(g_layer_list, key_name + strlen("toggle_"));
      if (layer == NULL) {
        layer = append_layer(&g_layer_list, new_layer(key_name + strlen("toggle_")));
      }
      append_layer_conf(&g_remap_parsee->to_when_double_tap_lock_layer, new_layer_conf(layer, toggle_layer_lock));
    } else if (strncmp(key_name, "set_layer", strlen("set_layer")) == 0) {
      struct Layer *layer = find_layer(g_layer_list, key_name + strlen("set_"));
      if (layer == NULL) {
        layer = append_layer(&g_layer_list, new_layer(key_name + strlen("set_")));
      }
      append_layer_conf(&g_remap_parsee->to_when_double_tap_lock_layer, new_layer_conf(layer, set_layer_lock));
    } else if (strncmp(key_name, "reset_layer", strlen("reset_layer")) == 0) {
      struct Layer *layer = find_layer(g_layer_list, key_name + strlen("reset_"));
      if (layer == NULL) {
        layer = append_layer(&g_layer_list, new_layer(key_name + strlen("reset_")));
      }
      append_layer_conf(&g_remap_parsee->to_when_double_tap_lock_layer, new_layer_conf(layer, reset_layer_lock));
    } else {
      if (!key_def) {
        printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
        return 1;
      }
      if (g_remap_parsee->to_when_double_tap_lock == NULL) {
        g_remap_parsee->to_when_double_tap_lock = new_key_node(key_def);
      } else {
        append_key_node(g_remap_parsee->to_when_double_tap_lock, key_def);
      }
    }
  } else if (strncmp(line, "define_layer=", strlen("define_layer=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      g_layer_parsee = find_layer(g_layer_list, key_name);
      if (g_layer_parsee == NULL) {
        g_layer_parsee = append_layer(&g_layer_list, new_layer(key_name));
      }
    } else {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
  } else if (strncmp(line, "or_layer=", strlen("or_layer=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      if (g_layer_parsee == NULL) {
        printf("Config error (line %d): Incomplete layer definition.\n"
               "Each layer definition must start with a 'define_layer'.\n",
               linenum);
        return 1;
      } else {
        struct Layer *master_layer = find_layer(g_layer_list, key_name);
        if (master_layer == NULL) {
          master_layer = append_layer(&g_layer_list, new_layer(key_name));
        }
        append_layer_node(&g_layer_parsee->or_master_layers, new_layer_node(master_layer));
        append_layer_node(&master_layer->slave_layers, new_layer_node(g_layer_parsee));
      }
    } else {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
  } else if (strncmp(line, "and_layer=", strlen("and_layer=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      if (g_layer_parsee == NULL) {
        printf("Config error (line %d): Incomplete layer definition.\n"
               "Each layer definition must start with a 'define_layer'.\n",
               linenum);
        return 1;
      } else {
        struct Layer *master_layer = find_layer(g_layer_list, key_name);
        if (master_layer == NULL) {
          master_layer = append_layer(&g_layer_list, new_layer(key_name));
        }
        append_layer_node(&g_layer_parsee->and_master_layers, new_layer_node(master_layer));
        append_layer_node(&master_layer->slave_layers, new_layer_node(g_layer_parsee));
      }
    } else {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
  } else if (strncmp(line, "and_not_layer=", strlen("and_not_layer=")) == 0) {
    if (strncmp(key_name, "layer", strlen("layer")) == 0) {
      if (g_layer_parsee == NULL) {
        printf("Config error (line %d): Incomplete layer definition.\n"
               "Each layer definition must start with a 'define_layer'.\n",
               linenum);
        return 1;
      } else {
        struct Layer *master_layer = find_layer(g_layer_list, key_name);
        if (master_layer == NULL) {
          master_layer = append_layer(&g_layer_list, new_layer(key_name));
        }
        append_layer_node(&g_layer_parsee->and_not_master_layers, new_layer_node(master_layer));
        append_layer_node(&master_layer->slave_layers, new_layer_node(g_layer_parsee));
      }
    } else {
      printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
      return 1;
    }
  } else {
    after_eq[0] = 0;
    printf("Config error (line %d): Invalid setting '%s'.\n", linenum, line);
    return 1;
  }

  return 0;
}
