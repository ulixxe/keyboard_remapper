#ifndef MOUSE_H
#define MOUSE_H

#include <windows.h> // HANDLE
#include "keyboard_remapper.h"
#include "input_buffer.h"

enum {
  MS_U = 1, // Move up.
  MS_D,     // Move down.
  MS_L,     // Move left.
  MS_R,     // Move right.
  MS_F,     // Move forward.
  MS_B,     // Move backward.
  MS_S_L,   // Steer left (counter-clockwise).
  MS_S_R,   // Steer right (clockwise).
  MS_W_U,   // Mouse wheel up.
  MS_W_D,   // Mouse wheel down.
  MS_W_L,   // Mouse wheel left.
  MS_W_R,   // Mouse wheel right.
  MS_BTN1,  // Press mouse button 1.
  MS_BTN2,  // Press mouse button 2.
  MS_BTN3,  // Press mouse button 3.
  MS_BTN4,  // Press mouse button 4.
  MS_BTN5,  // Press mouse button 5.
  MS_BTNS,  // Press the selected mouse button.
  MS_HLDS,  // Hold the selected mouse button.
  MS_RELS,  // Release the selected mouse button.
  MS_SEL1,  // Select mouse button 1.
  MS_SEL2,  // Select mouse button 2.
  MS_SEL3,  // Select mouse button 3.
  MS_SEL4,  // Select mouse button 4.
  MS_SEL5,  // Select mouse button 5.
};

void mouse_emulation(int keycode,
                     enum Direction direction,
                     int remap_id,
                     struct InputBuffer *input_buffer);

extern HANDLE g_hMouseTimer;
extern int g_mouse_timer;

#endif // MOUSE_H
