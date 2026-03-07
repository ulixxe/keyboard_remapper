#ifndef TRAY_H
#define TRAY_H

#include <windows.h>

/**
 * Initialize the tray icon.  Call once after your hooks are installed
 * and just before entering your GetMessage() loop.
 *
 * @return Tray handle
 */
HWND Tray_Init(void);

/**
 * Remove the tray icon and destroy the hidden window.
 * Call after your message loop exits (before process termination).
 *
 * @param Tray handle
 */
void Tray_Cleanup(HWND hwnd);

#endif // TRAY_H
