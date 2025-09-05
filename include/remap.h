#ifndef REMAP_H
#define REMAP_H

#include <windows.h> // DWORD ULONG_PTR
#include "keyboard_remapper.h"
#include "input_buffer.h"

int handle_input(
                 int scan_code,
                 int virt_code,
                 enum Direction direction,
                 DWORD time,
                 int is_injected,
                 DWORD flags,
                 ULONG_PTR dwExtraInfo,
                 struct InputBuffer *input_buffer);

void unlock_all(struct InputBuffer *input_buffer);

extern DWORD g_last_input;

#endif // REMAP_H
