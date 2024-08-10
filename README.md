## keyboard_remapper

**keyboard_remapper** remaps keyboard keys on Windows using the API [LowLevelKeyboardProc](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc) and [LowLevelMouseProc](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelmouseproc), which are low-level keyboard and mouse hook procedures that intercept key events. It also uses the API [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput) to synthesize keystrokes, mouse motions, and button clicks.

**keyboard_remapper** does not store any settings permanently on Windows. The key remapping occurs in real time, so if the executable is terminated, the keyboard returns to its default behavior. If a key becomes stuck because **keyboard_remapper** was closed while the key was pressed, simply pressing and releasing the key will resolve the issue.

The idea for **keyboard_remapper** is inspired by the excellent work of **ililim** on his [dual-key-remap](https://github.com/ililim/dual-key-remap). The main features of **keyboard_remapper** are:

1. Dual-role keys with tap timeout and auto-repeat keypress.
2. Hold delay for dual-role keys to prevent accidental activation of the hold functionality during typing.
3. Remapping to multiple keys.
4. Remapping for double-tap and tap&press actions.
5. Remap locking.
6. Layers, incorporating layer lock concepts from [Pascal Getreuer](https://getreuer.info/posts/keyboards/layer-lock/index.html).
7. Mouse emulation, inspired by the Orbital Mouse ideas from [Pascal Getreuer](https://getreuer.info/posts/keyboards/orbital-mouse/index.html).

My use case for **keyboard_remapper** involves creating lower-level keyboard remappings, specifically to remap the spacebar to function as both the space key and the right Ctrl key. Additionally, I set up a layer for Vi-style hjkl movements. On top of **keyboard_remapper**, I use [AutoHotkey](https://www.autohotkey.com/) for managing hotkeys and hotstrings, as well as for implementing Emacs key bindings universally with [ewow](https://github.com/zk-phi/ewow).

## keyboard_remapper configuration

The remappings for **keyboard_remapper** are configured through a `config.txt` file, which is located in the same directory as the **keyboard_remapper** executable. This allows for easy customization of key behaviors and settings.

### Dual-role key

Configuration example:

```
remap_key=SPACE
when_alone=SPACE
with_other=RIGHT_CTRL
```

Explanation:

- **`remap_key`**: This is the key you want to remap, in this case, the SPACE key.
- **`when_alone`**: The action triggered when `remap_key` is pressed by itself.
- **`with_other`**: The action triggered if another key is pressed while `remap_key` is held down.

Dual-role key timings in **keyboard_remapper** are managed using two global settings expressed in milliseconds:

- **`hold_delay`**: This defines the duration that starts from when the `remap_key` is pressed. If another key is pressed within this time frame, the action associated with `when_alone` is triggered instead of the expected hold action.

- **`tap_timeout`**: This setting typically defines the maximum time allowed for a tap action before it is considered a hold.

These settings provide fine control over how quickly the system distinguishes between a tap and a hold, allowing for a more customized user experience.

### Tap&press

In **keyboard_remapper**, you can set up a tap&press remapping using the following configuration example:

```
remap_key=SPACE
when_alone=SPACE
with_other=RIGHT_CTRL
when_doublepress=LEFT_ALT
```

Explanation:

- **`when_doublepress`**: If the `remap_key` is tapped and then held down within the duration specified by global setting `doublepress_timeout` (in milliseconds), the action associated with this setting (LEFT_ALT) will be triggered.

This setup allows for versatile key behavior based on how quickly the key is pressed.

If the `when_doublepress` action is not defined in your **keyboard_remapper** configuration, the default behavior will be to treat a tap&press as a repetition of the action specified in `when_alone`. This allows for normal key repetition as defined by the operating system (Windows), meaning that if the key is held down, it will continue to repeat the action until the key is released.

### Key lock

In **keyboard_remapper**, you can implement key locking behavior with the following configuration example:

```
remap_key=LEFT_SHIFT
when_alone=LEFT_SHIFT
when_tap_lock=LEFT_SHIFT
when_doublepress=LEFT_CTRL
when_double_tap_lock=LEFT_CTRL
```

Explanation:

- **`when_tap_lock`**: If the `remap_key` is tapped, the assigned key (LEFT_SHIFT) is locked in that state until it is tapped again. This allows users to  maintain the shift state without needing to hold the key.
- **`when_double_tap_lock`**: If the `remap_key` is double tapped, the assigned key (LEFT_CTRL) is locked in that state until a subsequent double tap of the `remap_key`. This provides a way to maintain control functionality without holding the key.

This configuration enhances the flexibility of key usage, allowing for both temporary and persistent key states based on user input patterns.

### Layers

In **keyboard_remapper**, layers allow for more complex key remappings, enabling users to switch between different sets of key configurations easily. This feature is particularly useful for different contexts, such as gaming, programming, or general typing.

Configuration example:

```
remap_key=KEY_H
layer=layer_vi
when_alone=LEFT
```

Explanation:

- **`layer`**: The `remap_key` remapping is enabled if the layer specified by `layer` is active. The name of specified layer has to start with "layer". In this example its name is "layer_vi".

Layers can be activated momentarily or permanently:

1. **Momentary Activation**

   A layer can be activated temporarily by holding down one or more specific keys. While the key is pressed, the remappings associated with that layer will be in effect.

   Configuration example:

   ```
   remap_key=LEFT_CTRL
   when_press=layer_vi
   when_doublepress=layer_mouse
   ```

   Explanation:

   - **`when_press`**: While the `remap_key` is held down, the layer specified by `when_press` is active.
   - **`when_doublepress`**: While the `remap_key` is tap&pressed, the layer specified by `when_doublepress` is active.

   To enable a layer with multiple key presses it is possibile to use `define_layer` as in the following configuration example:

   ```
   define_layer=layer_vi
   and_layer=layer_ctrl
   and_layer=layer_win
   and_not_layer=layer_shift
   ```

   Explanation:

   - **`define_layer=layer_vi`**: This line specifies that you are defining the `layer_vi`. This is the primary layer that will be activated when the conditions are met.
   - **`and_layer=layer_ctrl`**: This indicates that the `layer_ctrl` must also be active for `layer_vi` to be enabled. This allows you to combine multiple layers for more complex key remapping.
   - **`and_layer=layer_win`**: Similarly, this specifies that `layer_win` must also be active to enable `layer_vi`. This means that all specified layers must be active simultaneously for the remappings of `layer_vi` to take effect.
   - **`and_not_layer=layer_shift`**: This setting specifies that `layer_shift` must **not** be active to enable `layer_vi`.

2. **Permanent Activation (Lock System)**

   A layer can also be activated  permanently using a lock mechanism. This means that once the layer is  activated, all remappings associated with it remain in effect until the  layer is explicitly deactivated.

   Configuration example:

   ```
   remap_key=RIGHT_WIN
   when_tap_lock=set_layer_vi
   
   remap_key=RIGHT_WIN
   layer=layer_vi
   when_tap_lock=reset_layer_vi
   when_tap_lock=set_layer_mouse
   
   remap_key=RIGHT_WIN
   layer=layer_mouse
   when_tap_lock=reset_layer_mouse
   ```

   Explanation:

   - **`when_tap_lock=set_layer_vi`**: This setting lock in active state the `layer_vi` layer.
   - **`when_tap_lock=reset_layer_vi`**: This setting release the lock of `layer_vi` layer.

   Beside `set_` and `reset_` is available `toggle_` to toggle the lock state of specified layer.

   `when_double_tap_lock` can be used to change lock state of specified layer with `remap_key` double tap.
   
   In a key mapping, it is possible to specify multiple `when_tap_lock` and `when_double_tap_lock` actions.
   
   Layers can also be locked using the concept from [Pascal Getreuer](https://getreuer.info/posts/keyboards/layer-lock/index.html). In this case, a key is used to lock and unlock the same layer.
   
   Configuration example:
   
   ```
   remap_key=SPACE
   layer=layer_vi
   when_tap_lock=toggle_layer_vi
   ```
   
   Explanation:
   
   - **`layer=layer_vi`**: The lock mechanism is enabled by the `layer_vi`.
   - **`when_tap_lock=toggle_layer_vi`**: When the SPACE key is tapped, it locks the `layer_vi`. A subsequent tap of the SPACE key unlocks the `layer_vi`.

### Mouse emulation

In **`keyboard_remapper`** keyboard keys can be remapped to mouse events.

Configuration example:

```
remap_key=KEY_I
layer=layer_mouse
when_alone=MOUSE_LBUTTON
```

Explanation:

- **`when_alone=MOUSE_LBUTTON`**: When the `layer_mouse` is active, the I key is mapped to the mouse left button.

Mappable mouse events:

- Up, Down, Left and Right movements: `MOUSE_UP`, `MOUSE_DOWN`, `MOUSE_LEFT`, `MOUSE_RIGHT`.
- Orbital mouse movements (from [Pascal Getreuer](https://getreuer.info/posts/keyboards/orbital-mouse/index.html)): `MOUSE_FORWARD`, `MOUSE_BACKWARD`, `MOUSE_STEER_LEFT`, `MOUSE_STEER_RIGHT`.
- Mouse wheel rotations: `MOUSE_WHEEL_UP`, `MOUSE_WHEEL_DOWN`, `MOUSE_WHEEL_LEFT`, `MOUSE_WHEEL_RIGHT`.
- Mouse buttons: `MOUSE_LBUTTON`, `MOUSE_RBUTTON`, `MOUSE_MBUTTON`, `MOUSE_XBUTTON1`, `MOUSE_XBUTTON2`.
- Selection of a mouse button: `MOUSE_LBUTTON_SEL`, `MOUSE_RBUTTON_SEL`, `MOUSE_MBUTTON_SEL`, `MOUSE_XBUTTON1_SEL`, `MOUSE_XBUTTON2_SEL`.
- Selected mouse button events: `MOUSE_SBUTTON`, `MOUSE_SHOLD`, `MOUSE_SRELEASE`.

### Remapping to multiple keys

In **`keyboard_remapper`**, a key can be remapped to multiple keys.

Configuration example:

```
remap_key=RIGHT_ALT
when_alone=CTRL
when_alone=ALT
when_alone=SHIFT
when_alone=LEFT_WIN
```

Explanation:

The Right ALT key is mapped to multiple keys: CTRL, ALT, SHIFT, and Left WIN.

The key press sequence follows the order of the `when_alone` lines. Whereas the key release sequence occurs in reverse order.

It is possible to map multiple keys for `when_alone`, `with_other`, `when_doublepress`, `when_tap_lock`, and `when_double_tap_lock`.

