// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file orbital_mouse.c
 * @brief Orbital Mouse implementation
 *
 * For full documentation, see
 * <https://getreuer.info/posts/keyboards/orbital-mouse>
 */

#include <windows.h>
#include "mouse.h"
#include "keyboard_remapper.h"
#include "input_buffer.h"
#include "config.h"
#include "debug.h"

#ifndef ORBITAL_MOUSE_RADIUS
#define ORBITAL_MOUSE_RADIUS 36
#endif  // ORBITAL_MOUSE_RADIUS
#ifndef ORBITAL_MOUSE_WHEEL_SPEED
#define ORBITAL_MOUSE_WHEEL_SPEED 1.0
#endif  // ORBITAL_MOUSE_WHEEL_SPEED
#ifndef ORBITAL_MOUSE_SPEED_CURVE
#define ORBITAL_MOUSE_SPEED_CURVE                                   \
  {24, 24, 24, 32, 58, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66}
//     |               |               |               |           |
// t = 0.000           1.024           2.048           3.072       3.840 s
#endif  // ORBITAL_MOUSE_SPEED_CURVE
#ifndef ORBITAL_MOUSE_INTERVAL_MS
#define ORBITAL_MOUSE_INTERVAL_MS 16
#endif  // ORBITAL_MOUSE_INTERVAL_MS

#if !(0 <= ORBITAL_MOUSE_RADIUS && ORBITAL_MOUSE_RADIUS <= 63)
#error "Invalid ORBITAL_MOUSE_RADIUS. Value must be in [0, 63]."
#endif

enum {
  /** Number of intervals in speed curve table. */
  NUM_SPEED_CURVE_INTERVALS = 16,
};

typedef struct {
  int x;
  int y;
  int v;
  int h;
} report_mouse_t;

static const int init_speed_curve[NUM_SPEED_CURVE_INTERVALS] =
  ORBITAL_MOUSE_SPEED_CURVE;
static struct MouseState {
  report_mouse_t report;
  // Current speed curve, should point to a table of 16 values.
  const int *speed_curve;
  // Fractional displacement of the cursor as Q7.8 values.
  double x;
  double y;
  // Fractional displacement of the mouse wheel as Q9.6 values.
  double wheel_x;
  double wheel_y;
  // Current cursor movement speed as a Q9.6 value.
  double speed;
  // Bitfield tracking which movement keys are currently held.
  int held_keys;
  // Cursor movement time, counted in number of intervals.
  int move_t;
  // Cursor movement direction, 1 => up, -1 => down.
  int move_v;
  // Cursor movement direction, 1 => left, -1 => right.
  int move_h;
  // Cursor movement direction, 1 => forward, -1 => backward.
  int move_dir;
  // Steering direction, 1 => counter-clockwise, -1 => clockwise.
  int steer_dir;
  // Mouse wheel movement directions.
  int wheel_x_dir;
  int wheel_y_dir;
  // Current heading direction, 0 => up, 16 => left, 32 => down, 48 => right.
  double angle;
  // Bitfield tracking which buttons are currently held.
  int buttons;
  int last_buttons;
  // Selected mouse button as a base-0 index.
  int selected_button;
} state = {.move_v = 0,
           .move_h = 0,
           .move_dir = 0,
           .last_buttons = 0,
           .selected_button = 0,
           .speed_curve = init_speed_curve};

HANDLE g_hMouseTimer = NULL;
int g_mouse_timer = 0;

static void set_orbital_mouse_speed_curve(const int *speed_curve) {
  state.speed_curve = (speed_curve != NULL) ? speed_curve : init_speed_curve;
}

static void buttons_send(struct MouseState *state, int remap_id, struct InputBuffer *input_buffer) {
  uint32_t n, tail;
  int index;
  n = input_buffer_move_prod_head(input_buffer, &tail);
  index = tail & INPUT_BUFFER_MASK;
  if (n == 0) {
    if (g_debug) debug_print(ANSI_RED, "Error: input buffer is full!\n");
    debug_file("Error: input buffer is full!");
    return;
  }
  INPUT *out = &input_buffer->inputs[index];
  //ZeroMemory(out, sizeof(INPUT));
  out->type = INPUT_MOUSE;
  out->mi.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID | remap_id;

  // Left button
  if ((state->buttons ^ state->last_buttons) & (1 << 0)) {
    if (state->buttons & (1 << 0)) {
      out->mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
    } else {
      out->mi.dwFlags |= MOUSEEVENTF_LEFTUP;
    }
  }

  // Right button
  if ((state->buttons ^ state->last_buttons) & (1 << 1)) {
    if (state->buttons & (1 << 1)) {
      out->mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
    } else {
      out->mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
    }
  }

  // Middle button
  if ((state->buttons ^ state->last_buttons) & (1 << 2)) {
    if (state->buttons & (1 << 2)) {
      out->mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
    } else {
      out->mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
    }
  }

  // Note: Next buttons use mouseData field so they cannot be sent in one call

  // Button 4
  if ((state->buttons ^ state->last_buttons) & (1 << 3)) {
    if (state->buttons & (1 << 3)) {
      out->mi.dwFlags |= MOUSEEVENTF_XDOWN;
      out->mi.mouseData |= XBUTTON1;
    } else {
      out->mi.dwFlags |= MOUSEEVENTF_XUP;
      out->mi.mouseData |= XBUTTON1;
    }
  }

  // Button 5
  if ((state->buttons ^ state->last_buttons) & (1 << 4)) {
    // If mouseData is used by button 4, send input and clear mi
    if ((state->buttons ^ state->last_buttons) & (1 << 3)) {
      input_buffer_update_tail(&input_buffer->prod, tail, n);
      n = input_buffer_move_prod_head(input_buffer, &tail);
      index = tail & INPUT_BUFFER_MASK;
      if (n == 0) {
        if (g_debug) debug_print(ANSI_RED, "Error: input buffer is full!\n");
        debug_file("Error: input buffer is full!");
        return;
      }
      out = &input_buffer->inputs[index];
      //ZeroMemory(out, sizeof(INPUT));
      out->type = INPUT_MOUSE;
      out->mi.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID | remap_id;
    }
    if (state->buttons & (1 << 4)) {
      out->mi.dwFlags |= MOUSEEVENTF_XDOWN;
      out->mi.mouseData |= XBUTTON2;
    } else {
      out->mi.dwFlags |= MOUSEEVENTF_XUP;
      out->mi.mouseData |= XBUTTON2;
    }
  }
  input_buffer_update_tail(&input_buffer->prod, tail, n);
}

static void set_orbital_mouse_angle(double angle) {
  state.x += ORBITAL_MOUSE_RADIUS * sin(state.angle);
  state.y += ORBITAL_MOUSE_RADIUS * cos(state.angle);
  state.angle = angle;
  state.x -= ORBITAL_MOUSE_RADIUS * sin(angle);
  state.y -= ORBITAL_MOUSE_RADIUS * cos(angle);
}

static void move_send(struct MouseState *state, int remap_id, struct InputBuffer *input_buffer) {
  uint32_t n, tail;
  int index;
  n = input_buffer_move_prod_head(input_buffer, &tail);
  index = tail & INPUT_BUFFER_MASK;
  if (n == 0) {
    if (g_debug) debug_print(ANSI_RED, "Error: input buffer is full!\n");
    debug_file("Error: input buffer is full!");
    return;
  }
  INPUT *out = &input_buffer->inputs[index];
  //ZeroMemory(out, sizeof(INPUT));
  out->type = INPUT_MOUSE;
  out->mi.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID | remap_id;

  // Update position if moving.
  if (state->move_dir || state->move_h || state->move_v) {
    // Update speed, interpolated from speed_curve.
    if (state->move_t <= 16 * (NUM_SPEED_CURVE_INTERVALS - 1)) {
      if (state->move_t == 0) {
        state->speed = state->speed_curve[0] * 16;
      } else {
        const int i = (state->move_t - 1) / 16;
        state->speed += state->speed_curve[i + 1]
          - state->speed_curve[i];
      }

      ++state->move_t;
    }
    if (state->move_dir) {
      state->x -= state->move_dir * state->speed * sin(state->angle) /64;
      state->y -= state->move_dir * state->speed * cos(state->angle) /64;
    }
    if (state->move_h) {
      state->x -= state->move_h * state->speed /64;
    }
    if (state->move_v) {
      state->y -= state->move_v * state->speed /64;
    }
  }

  // Update heading angle if steering.
  if (state->steer_dir) {
    set_orbital_mouse_angle(state->angle + (double)state->steer_dir/10);
  }

  // Update mouse wheel if active.
  if (state->wheel_x_dir || state->wheel_y_dir) {
    state->wheel_x -= state->wheel_x_dir * ORBITAL_MOUSE_WHEEL_SPEED * WHEEL_DELTA;
    state->wheel_y += state->wheel_y_dir * ORBITAL_MOUSE_WHEEL_SPEED * WHEEL_DELTA;
  }

  // Set whole part of movement deltas in report and retain fractional parts.
  state->report.x = (int)state->x;
  state->report.y = (int)state->y;
  state->x -= (double)state->report.x;
  state->y -= (double)state->report.y;

  // Set whole part of movement deltas in report and retain fractional parts.
  state->report.h = (int)state->wheel_x;
  state->report.v = (int)state->wheel_y;
  state->wheel_x -= (double)state->report.h;
  state->wheel_y -= (double)state->report.v;

  // Mouse position
  if (state->report.x != 0 || state->report.y != 0) {
    out->mi.dx = state->report.x;
    out->mi.dy = state->report.y;
    out->mi.dwFlags |= MOUSEEVENTF_MOVE;
  }

  // Mouse wheel
  if (state->report.v != 0) {
    out->mi.mouseData = state->report.v;
    out->mi.dwFlags |= MOUSEEVENTF_WHEEL;
  }

  // Mouse horizontal wheel
  if (state->report.h != 0) {
    // If mouseData is used by wheel, send input and clear mi
    if (state->report.v != 0) {
      input_buffer_update_tail(&input_buffer->prod, tail, n);
      n = input_buffer_move_prod_head(input_buffer, &tail);
      index = tail & INPUT_BUFFER_MASK;
      if (n == 0) {
        if (g_debug) debug_print(ANSI_RED, "Error: input buffer is full!\n");
        debug_file("Error: input buffer is full!");
        return;
      }
      out = &input_buffer->inputs[index];
      //ZeroMemory(out, sizeof(INPUT));
      out->type = INPUT_MOUSE;
      out->mi.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID | remap_id;
    }
    out->mi.mouseData = state->report.h;
    out->mi.dwFlags |= MOUSEEVENTF_HWHEEL;
  }
  input_buffer_update_tail(&input_buffer->prod, tail, n);
}

static VOID CALLBACK move_callback(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
  int mouse_timer = *(int *)lpParam;
  if (mouse_timer) {
    move_send(&state, 0, &g_input_buffer);
    if (!input_buffer_empty(&g_input_buffer))
      SetEvent(g_hEvent);
  }
}

/** Presses mouse button i, with i being a base-0 index. */
static void press_mouse_button(int i, enum Direction direction) {
  if (i >= 5) {
    i = state.selected_button;
  }
  if (direction == UP) {
    state.buttons &= ~(1 << i);
  } else {
    state.buttons |= (1 << i);
  }
}

static int get_dir_from_held_keys(int bit_shift) {
  static const int dir[4] = {0, 1, -1, 0};
  return dir[(state.held_keys >> bit_shift) & 3];
}

void mouse_emulation(int keycode, enum Direction direction, int remap_id, struct InputBuffer *input_buffer) {
  int held_mask = 0;
  switch (keycode) {
  case MS_U: held_mask = 1; break;
  case MS_D: held_mask = 2; break;
  case MS_L: held_mask = 4; break;
  case MS_R: held_mask = 8; break;
  case MS_F: held_mask = 16; break;
  case MS_B: held_mask = 32; break;
  case MS_S_L: held_mask = 64; break;
  case MS_S_R: held_mask = 128; break;
  case MS_W_U: held_mask = 256; break;
  case MS_W_D: held_mask = 512; break;
  case MS_W_L: held_mask = 1024; break;
  case MS_W_R: held_mask = 2048; break;
  }
  if (held_mask != 0) {
    // Update `held_keys` bitfield.
    if (direction == UP) {
      state.held_keys &= ~held_mask;
    } else {
      state.held_keys |= held_mask;
    }

    // Update cursor movement direction.
    int dir = get_dir_from_held_keys(0);
    if (state.move_v != dir) {
      state.move_v = dir;
      state.move_t = 0;
    }
    dir = get_dir_from_held_keys(2);
    if (state.move_h != dir) {
      state.move_h = dir;
      state.move_t = 0;
    }
    dir = get_dir_from_held_keys(4);
    if (state.move_dir != dir) {
      state.move_dir = dir;
      state.move_t = 0;
    }
    // Update steering direction.
    state.steer_dir = get_dir_from_held_keys(6);
    // Update wheel movement.
    state.wheel_y_dir = get_dir_from_held_keys(8);
    state.wheel_x_dir = get_dir_from_held_keys(10);

    if (state.move_v || state.move_h || state.move_dir ||
        state.steer_dir || state.wheel_x_dir || state.wheel_y_dir) {
      if (!g_mouse_timer){
        move_send(&state, remap_id, input_buffer);
      }
    } else {
      g_mouse_timer = 0;
    }
    if (g_mouse_timer) {
      if (g_hMouseTimer == NULL) {
        if (!CreateTimerQueueTimer(&g_hMouseTimer, g_hTimerQueue, (WAITORTIMERCALLBACK)move_callback,
                                   &g_mouse_timer, 0, ORBITAL_MOUSE_INTERVAL_MS, 0)) {
          DEBUG(-1, debug_print(ANSI_RED, "CreateTimerQueueTimer failed (%d)\n", GetLastError()));
        }
      }
    } else {
      if (g_hMouseTimer) {
        if (!DeleteTimerQueueTimer(g_hTimerQueue, g_hMouseTimer, INVALID_HANDLE_VALUE))
          DEBUG(-1, debug_print(ANSI_RED, "DeleteTimerQueueTimer failed (%d)\n", GetLastError()));
        g_hMouseTimer = NULL;
      }
    }
    if (state.move_v || state.move_h || state.move_dir ||
        state.steer_dir || state.wheel_x_dir || state.wheel_y_dir)
      g_mouse_timer = 1;
  } else {
    switch (keycode) {
    case MS_BTN1:
    case MS_BTN2:
    case MS_BTN3:
    case MS_BTN4:
    case MS_BTN5:
      press_mouse_button(keycode - MS_BTN1, direction);
      break;
    case MS_BTNS:
      press_mouse_button(255, direction);
      break;
    case MS_HLDS:
      if (direction != UP) {
        press_mouse_button(255, DOWN);
      }
      break;
    case MS_RELS:
      if (direction != UP) {
        press_mouse_button(255, UP);
      }
      break;
    case MS_SEL1:
    case MS_SEL2:
    case MS_SEL3:
    case MS_SEL4:
    case MS_SEL5:
      if (direction != UP) {
        state.selected_button = keycode - MS_SEL1;
        // Reset buttons when switching selection.
        state.buttons = 0;
      }
      break;
    }
    if (state.buttons != state.last_buttons) {
      buttons_send(&state, remap_id, input_buffer);
      state.last_buttons = state.buttons;
    }
  }
}
