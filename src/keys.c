#include "keys.h"
#include "mouse.h"

#define VK_LEFT_CTRL    0xA2
#define VK_RIGHT_CTRL   0xA3
#define VK_LEFT_ALT     0xA4
#define VK_RIGHT_ALT    0xA5
#define VK_LEFT_SHIFT   0xA0
#define VK_RIGHT_SHIFT  0xA1
#define VK_LEFT_WIN     0x5B
#define VK_RIGHT_WIN    0x5C

#define VK_BACKSPACE    0x08
#define VK_CAPSLOCK     0x14
#define VK_ENTER        0x0D
#define VK_ESCAPE       0x1B
#define VK_SPACE        0x20
#define VK_TAB          0x09

#define VK_LEFT         0x25
#define VK_UP           0x26
#define VK_RIGHT        0x27
#define VK_DOWN         0x28

#define VK_F1           0x70
#define VK_F2           0x71
#define VK_F3           0x72
#define VK_F4           0x73
#define VK_F5           0x74
#define VK_F6           0x75
#define VK_F7           0x76
#define VK_F8           0x77
#define VK_F9           0x78
#define VK_F10          0x79
#define VK_F11          0x7A
#define VK_F12          0x7B
#define VK_F13          0x7C
#define VK_F14          0x7D
#define VK_F15          0x7E
#define VK_F16          0x7F
#define VK_F17          0x80
#define VK_F18          0x81
#define VK_F19          0x82
#define VK_F20          0x83
#define VK_F21          0x84
#define VK_F22          0x85
#define VK_F23          0x86
#define VK_F24          0x87

#define VK_KEY_0        0x30
#define VK_KEY_1        0x31
#define VK_KEY_2        0x32
#define VK_KEY_3        0x33
#define VK_KEY_4        0x34
#define VK_KEY_5        0x35
#define VK_KEY_6        0x36
#define VK_KEY_7        0x37
#define VK_KEY_8        0x38
#define VK_KEY_9        0x39

#define VK_KEY_A        0x41
#define VK_KEY_B        0x42
#define VK_KEY_C        0x43
#define VK_KEY_D        0x44
#define VK_KEY_E        0x45
#define VK_KEY_F        0x46
#define VK_KEY_G        0x47
#define VK_KEY_H        0x48
#define VK_KEY_I        0x49
#define VK_KEY_J        0x4A
#define VK_KEY_K        0x4B
#define VK_KEY_L        0x4C
#define VK_KEY_M        0x4D
#define VK_KEY_N        0x4E
#define VK_KEY_O        0x4F
#define VK_KEY_P        0x50
#define VK_KEY_Q        0x51
#define VK_KEY_R        0x52
#define VK_KEY_S        0x53
#define VK_KEY_T        0x54
#define VK_KEY_U        0x55
#define VK_KEY_V        0x56
#define VK_KEY_W        0x57
#define VK_KEY_X        0x58
#define VK_KEY_Y        0x59
#define VK_KEY_Z        0x5A

#define VK_INSERT       0x2D
#define VK_DELETE       0x2E
#define VK_HOME         0x24
#define VK_END          0x23
#define VK_PAGE_UP      0x21
#define VK_PAGE_DOWN    0x22

#define VK_PRINT_SCREEN 0x2C
#define VK_NUMLOCK      0x90
#define VK_SCROLLLOCK   0x91
#define VK_PAUSE        0x13

#define VK_PLUS         0xBB
#define VK_COMMA        0xBC
#define VK_MINUS        0xBD
#define VK_PERIOD       0xBE

#define VK_US_SEMI      0xBA // ;: key on US keyboards
#define VK_US_SLASH     0xBF // /? key on US keyboards
#define VK_US_TILDE     0xC0 // `~ key on US keyboards

#define SK_LEFT_CTRL   0x1D
#define SK_RIGHT_CTRL  0xE01D
#define SK_LEFT_ALT    0x38
#define SK_RIGHT_ALT   0xE038
#define SK_LEFT_SHIFT  0x2A
#define SK_RIGHT_SHIFT 0x36
#define SK_LEFT_WIN    0xE05B
#define SK_RIGHT_WIN   0xE05C

#define SK_BACKSPACE   0x0E
#define SK_CAPSLOCK    0x3A
#define SK_ENTER       0x1C
#define SK_ESCAPE      0x01
#define SK_SPACE       0x39
#define SK_TAB         0x0F

#define SK_UP          0x48
#define SK_LEFT        0x4B
#define SK_RIGHT       0x4D
#define SK_DOWN        0x50

#define SK_F1          0x3B
#define SK_F2          0x3C
#define SK_F3          0x3D
#define SK_F4          0x3E
#define SK_F5          0x3F
#define SK_F6          0x40
#define SK_F7          0x41
#define SK_F8          0x42
#define SK_F9          0x43
#define SK_F10         0x44
#define SK_F11         0x57
#define SK_F12         0x58
#define SK_F13         0x64
#define SK_F14         0x65
#define SK_F15         0x66
#define SK_F16         0x67
#define SK_F17         0x68
#define SK_F18         0x69
#define SK_F19         0x6A
#define SK_F20         0x6B
#define SK_F21         0x6C
#define SK_F22         0x6D
#define SK_F23         0x6E
#define SK_F24         0x76

#define SK_KEY_0       0x0B
#define SK_KEY_1       0x02
#define SK_KEY_2       0x03
#define SK_KEY_3       0x04
#define SK_KEY_4       0x05
#define SK_KEY_5       0x06
#define SK_KEY_6       0x07
#define SK_KEY_7       0x08
#define SK_KEY_8       0x09
#define SK_KEY_9       0x0A

#define SK_KEY_A       0x1E
#define SK_KEY_B       0x30
#define SK_KEY_C       0x2E
#define SK_KEY_D       0x20
#define SK_KEY_E       0x12
#define SK_KEY_F       0x21
#define SK_KEY_G       0x22
#define SK_KEY_H       0x23
#define SK_KEY_I       0x17
#define SK_KEY_J       0x24
#define SK_KEY_K       0x25
#define SK_KEY_L       0x26
#define SK_KEY_M       0x32
#define SK_KEY_N       0x31
#define SK_KEY_O       0x18
#define SK_KEY_P       0x19
#define SK_KEY_Q       0x10
#define SK_KEY_R       0x13
#define SK_KEY_S       0x1F
#define SK_KEY_T       0x14
#define SK_KEY_U       0x16
#define SK_KEY_V       0x2F
#define SK_KEY_W       0x11
#define SK_KEY_X       0x2D
#define SK_KEY_Y       0x15
#define SK_KEY_Z       0x2C

#define SK_INSERT      0x52
#define SK_DELETE      0x53
#define SK_HOME        0x47
#define SK_END         0x4F
#define SK_PAGE_UP     0x49
#define SK_PAGE_DOWN   0x51

#define SK_PLUS        0x0D
#define SK_COMMA       0x33
#define SK_MINUS       0x0C
#define SK_PERIOD      0x34

#define SK_US_SEMI     0x27 // ;: key on US keyboards
#define SK_US_SLASH    0x35 // /? key on US keyboards
#define SK_US_TILDE    0x29 // `~ key on US keyboards

// See: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
static const KeyDef key_table[] = {
  /* name                  scan            virt             mod */
  {  "CTRL",               SK_LEFT_CTRL,   VK_LEFT_CTRL,    0x01  },
  {  "LEFT_CTRL",          SK_LEFT_CTRL,   VK_LEFT_CTRL,    0x01  },
  {  "RIGHT_CTRL",         SK_RIGHT_CTRL,  VK_RIGHT_CTRL,   0x02  },

  {  "SHIFT",              SK_LEFT_SHIFT,  VK_LEFT_SHIFT,   0x04  },
  {  "LEFT_SHIFT",         SK_LEFT_SHIFT,  VK_LEFT_SHIFT,   0x04  },
  {  "RIGHT_SHIFT",        SK_RIGHT_SHIFT, VK_RIGHT_SHIFT,  0x08  },

  {  "ALT",                SK_LEFT_ALT,    VK_LEFT_ALT,     0x10  },
  {  "LEFT_ALT",           SK_LEFT_ALT,    VK_LEFT_ALT,     0x10  },
  {  "RIGHT_ALT",          SK_RIGHT_ALT,   VK_RIGHT_ALT,    0x20  },

  {  "LEFT_WIN",           SK_LEFT_WIN,    VK_LEFT_WIN,     0x40  },
  {  "RIGHT_WIN",          SK_RIGHT_WIN,   VK_RIGHT_WIN,    0x80  },

  {  "BACKSPACE",          SK_BACKSPACE,   VK_BACKSPACE,    0x00  },
  {  "CAPSLOCK",           SK_CAPSLOCK,    VK_CAPSLOCK,     0x00  },
  {  "ENTER",              SK_ENTER,       VK_ENTER,        0x00  },
  {  "ESCAPE",             SK_ESCAPE,      VK_ESCAPE,       0x00  },
  {  "SPACE",              SK_SPACE,       VK_SPACE,        0x00  },
  {  "TAB",                SK_TAB,         VK_TAB,          0x00  },

  {  "UP",                 SK_UP,          VK_UP,           0x00  },
  {  "LEFT",               SK_LEFT,        VK_LEFT,         0x00  },
  {  "RIGHT",              SK_RIGHT,       VK_RIGHT,        0x00  },
  {  "DOWN",               SK_DOWN,        VK_DOWN,         0x00  },

  {  "F1",                 SK_F1,          VK_F1,           0x00  },
  {  "F2",                 SK_F2,          VK_F2,           0x00  },
  {  "F3",                 SK_F3,          VK_F3,           0x00  },
  {  "F4",                 SK_F4,          VK_F4,           0x00  },
  {  "F5",                 SK_F5,          VK_F5,           0x00  },
  {  "F6",                 SK_F6,          VK_F6,           0x00  },
  {  "F7",                 SK_F7,          VK_F7,           0x00  },
  {  "F8",                 SK_F8,          VK_F8,           0x00  },
  {  "F9",                 SK_F9,          VK_F9,           0x00  },
  {  "F10",                SK_F10,         VK_F10,          0x00  },
  {  "F11",                SK_F11,         VK_F11,          0x00  },
  {  "F12",                SK_F12,         VK_F12,          0x00  },
  {  "F13",                SK_F13,         VK_F13,          0x00  },
  {  "F14",                SK_F14,         VK_F14,          0x00  },
  {  "F15",                SK_F15,         VK_F15,          0x00  },
  {  "F16",                SK_F16,         VK_F16,          0x00  },
  {  "F17",                SK_F17,         VK_F17,          0x00  },
  {  "F18",                SK_F18,         VK_F18,          0x00  },
  {  "F19",                SK_F19,         VK_F19,          0x00  },
  {  "F20",                SK_F20,         VK_F20,          0x00  },
  {  "F21",                SK_F21,         VK_F21,          0x00  },
  {  "F22",                SK_F22,         VK_F22,          0x00  },
  {  "F23",                SK_F23,         VK_F23,          0x00  },
  {  "F24",                SK_F24,         VK_F24,          0x00  },

  {  "KEY_0",              SK_KEY_0,       VK_KEY_0,        0x00  },
  {  "KEY_1",              SK_KEY_1,       VK_KEY_1,        0x00  },
  {  "KEY_2",              SK_KEY_2,       VK_KEY_2,        0x00  },
  {  "KEY_3",              SK_KEY_3,       VK_KEY_3,        0x00  },
  {  "KEY_4",              SK_KEY_4,       VK_KEY_4,        0x00  },
  {  "KEY_5",              SK_KEY_5,       VK_KEY_5,        0x00  },
  {  "KEY_6",              SK_KEY_6,       VK_KEY_6,        0x00  },
  {  "KEY_7",              SK_KEY_7,       VK_KEY_7,        0x00  },
  {  "KEY_8",              SK_KEY_8,       VK_KEY_8,        0x00  },
  {  "KEY_9",              SK_KEY_9,       VK_KEY_9,        0x00  },

  {  "KEY_A",              SK_KEY_A,       VK_KEY_A,        0x00  },
  {  "KEY_B",              SK_KEY_B,       VK_KEY_B,        0x00  },
  {  "KEY_C",              SK_KEY_C,       VK_KEY_C,        0x00  },
  {  "KEY_D",              SK_KEY_D,       VK_KEY_D,        0x00  },
  {  "KEY_E",              SK_KEY_E,       VK_KEY_E,        0x00  },
  {  "KEY_F",              SK_KEY_F,       VK_KEY_F,        0x00  },
  {  "KEY_G",              SK_KEY_G,       VK_KEY_G,        0x00  },
  {  "KEY_H",              SK_KEY_H,       VK_KEY_H,        0x00  },
  {  "KEY_I",              SK_KEY_I,       VK_KEY_I,        0x00  },
  {  "KEY_J",              SK_KEY_J,       VK_KEY_J,        0x00  },
  {  "KEY_K",              SK_KEY_K,       VK_KEY_K,        0x00  },
  {  "KEY_L",              SK_KEY_L,       VK_KEY_L,        0x00  },
  {  "KEY_M",              SK_KEY_M,       VK_KEY_M,        0x00  },
  {  "KEY_N",              SK_KEY_N,       VK_KEY_N,        0x00  },
  {  "KEY_O",              SK_KEY_O,       VK_KEY_O,        0x00  },
  {  "KEY_P",              SK_KEY_P,       VK_KEY_P,        0x00  },
  {  "KEY_Q",              SK_KEY_Q,       VK_KEY_Q,        0x00  },
  {  "KEY_R",              SK_KEY_R,       VK_KEY_R,        0x00  },
  {  "KEY_S",              SK_KEY_S,       VK_KEY_S,        0x00  },
  {  "KEY_T",              SK_KEY_T,       VK_KEY_T,        0x00  },
  {  "KEY_U",              SK_KEY_U,       VK_KEY_U,        0x00  },
  {  "KEY_V",              SK_KEY_V,       VK_KEY_V,        0x00  },
  {  "KEY_W",              SK_KEY_W,       VK_KEY_W,        0x00  },
  {  "KEY_X",              SK_KEY_X,       VK_KEY_X,        0x00  },
  {  "KEY_Y",              SK_KEY_Y,       VK_KEY_Y,        0x00  },
  {  "KEY_Z",              SK_KEY_Z,       VK_KEY_Z,        0x00  },

  {  "INSERT",             SK_INSERT,      VK_INSERT,       0x00  },
  {  "DELETE",             SK_DELETE,      VK_DELETE,       0x00  },
  {  "HOME",               SK_HOME,        VK_HOME,         0x00  },
  {  "END",                SK_END,         VK_END,          0x00  },
  {  "PAGE_UP",            SK_PAGE_UP,     VK_PAGE_UP,      0x00  },
  {  "PAGE_DOWN",          SK_PAGE_DOWN,   VK_PAGE_DOWN,    0x00  },

  {  "PRINT_SCREEN",       0,              VK_PRINT_SCREEN, 0x00  },
  {  "NUMLOCK",            0,              VK_NUMLOCK,      0x00  },
  {  "SCROLLLOCK",         0,              VK_SCROLLLOCK,   0x00  },
  {  "PAUSE",              0,              VK_PAUSE,        0x00  },

  {  "PLUS",               SK_PLUS,        VK_PLUS,         0x00  },
  {  "COMMA",              SK_COMMA,       VK_COMMA,        0x00  },
  {  "MINUS",              SK_MINUS,       VK_MINUS,        0x00  },
  {  "PERIOD",             SK_PERIOD,      VK_PERIOD,       0x00  },

  {  "US_SEMI",            SK_US_SEMI,     VK_US_SEMI,      0x00  },
  {  "US_SLASH",           SK_US_SLASH,    VK_US_SLASH,     0x00  },
  {  "US_TILDE",           SK_US_TILDE,    VK_US_TILDE,     0x00  },

  {  "MOUSE_UP",           MS_U,           0,               0x00  },
  {  "MOUSE_DOWN",         MS_D,           0,               0x00  },
  {  "MOUSE_LEFT",         MS_L,           0,               0x00  },
  {  "MOUSE_RIGHT",        MS_R,           0,               0x00  },
  {  "MOUSE_FORWARD",      MS_F,           0,               0x00  },
  {  "MOUSE_BACKWARD",     MS_B,           0,               0x00  },
  {  "MOUSE_STEER_LEFT",   MS_S_L,         0,               0x00  },
  {  "MOUSE_STEER_RIGHT",  MS_S_R,         0,               0x00  },
  {  "MOUSE_WHEEL_UP",     MS_W_U,         0,               0x00  },
  {  "MOUSE_WHEEL_DOWN",   MS_W_D,         0,               0x00  },
  {  "MOUSE_WHEEL_LEFT",   MS_W_L,         0,               0x00  },
  {  "MOUSE_WHEEL_RIGHT",  MS_W_R,         0,               0x00  },
  {  "MOUSE_LBUTTON",      MS_BTN1,        0,               0x00  },
  {  "MOUSE_RBUTTON",      MS_BTN2,        0,               0x00  },
  {  "MOUSE_MBUTTON",      MS_BTN3,        0,               0x00  },
  {  "MOUSE_XBUTTON1",     MS_BTN4,        0,               0x00  },
  {  "MOUSE_XBUTTON2",     MS_BTN5,        0,               0x00  },
  {  "MOUSE_SBUTTON",      MS_BTNS,        0,               0x00  },
  {  "MOUSE_SHOLD",        MS_HLDS,        0,               0x00  },
  {  "MOUSE_SRELEASE",     MS_RELS,        0,               0x00  },
  {  "MOUSE_LBUTTON_SEL",  MS_SEL1,        0,               0x00  },
  {  "MOUSE_RBUTTON_SEL",  MS_SEL2,        0,               0x00  },
  {  "MOUSE_MBUTTON_SEL",  MS_SEL3,        0,               0x00  },
  {  "MOUSE_XBUTTON1_SEL", MS_SEL4,        0,               0x00  },
  {  "MOUSE_XBUTTON2_SEL", MS_SEL5,        0,               0x00  },
};

static const KeyDef nokey_table[] = {
  /* name                  scan            virt             mod */
  {  "<MOUSE INPUT>",         0,           MOUSE_DUMMY_VK,  0  },
  {  "<ZERO_CODE>",           0,           0x00,            0  },
  {  "<MOUSE_LEFT>",          0,           0x01,            0  },
  {  "<MOUSE_RIGHT>",         0,           0x02,            0  },
  {  "<CANCEL>",              0,           0x03,            0  },
  {  "<MOUSE_MIDDLE>",        0,           0x04,            0  },
  {  "<MOUSE_X1>",            0,           0x05,            0  },
  {  "<MOUSE_X2>",            0,           0x06,            0  },
  {  "<CLEAR>",               0,           0x0C,            0  },
  {  "<SHIFT_NO_DIR>",        0,           0x10,            0  },
  {  "<CTRL_NO_DIR>",         0,           0x11,            0  },
  {  "<ALT_NO_DIR>",          0,           0x12,            0  },
  {  "<IME_KANA_OR_HANGUL>",  0,           0x15,            0  },
  {  "<IME_ON>",              0,           0x16,            0  },
  {  "<IME_JUNJA>",           0,           0x17,            0  },
  {  "<IME_FINAL>",           0,           0x18,            0  },
  {  "<IME_HANJA_OR_KANJI>",  0,           0x19,            0  },
  {  "<IME_OFF>",             0,           0x1A,            0  },
  {  "<IME_CONVERT>",         0,           0x1C,            0  },
  {  "<IME_NONCONVERT>",      0,           0x1D,            0  },
  {  "<IME_ACCEPT>",          0,           0x1E,            0  },
  {  "<IME_MODE_CHANGE>",     0,           0x1F,            0  },
  {  "<SELECT>",              0,           0x29,            0  },
  {  "<PRINT>",               0,           0x2A,            0  },
  {  "<EXECUTE>",             0,           0x2B,            0  },
  {  "<HELP>",                0,           0x2F,            0  },
  {  "<APPS>",                0,           0x5D,            0  },
  {  "<SLEEP>",               0,           0x5F,            0  },
  {  "<NUMPAD_0>",            0,           0x60,            0  },
  {  "<NUMPAD_1>",            0,           0x61,            0  },
  {  "<NUMPAD_2>",            0,           0x62,            0  },
  {  "<NUMPAD_3>",            0,           0x63,            0  },
  {  "<NUMPAD_4>",            0,           0x64,            0  },
  {  "<NUMPAD_5>",            0,           0x65,            0  },
  {  "<NUMPAD_6>",            0,           0x66,            0  },
  {  "<NUMPAD_7>",            0,           0x67,            0  },
  {  "<NUMPAD_8>",            0,           0x68,            0  },
  {  "<NUMPAD_9>",            0,           0x69,            0  },
  {  "<MULTIPLY>",            0,           0x6A,            0  },
  {  "<ADD>",                 0,           0x6B,            0  },
  {  "<SEPARATOR>",           0,           0x6C,            0  },
  {  "<SUBTRACT>",            0,           0x6D,            0  },
  {  "<DECIMAL>",             0,           0x6E,            0  },
  {  "<DIVIDE>",              0,           0x6F,            0  },
  {  "<BROWSER_BACK>",        0,           0xA6,            0  },
  {  "<BROWSER_FORWARD>",     0,           0xA7,            0  },
  {  "<BROWSER_REFRESH>",     0,           0xA8,            0  },
  {  "<BROWSER_STOP>",        0,           0xA9,            0  },
  {  "<BROWSER_SEARCH>",      0,           0xAA,            0  },
  {  "<BROWSER_FAVORITES>",   0,           0xAB,            0  },
  {  "<BROWSER_HOME>",        0,           0xAC,            0  },
  {  "<VOLUME_MUTE>",         0,           0xAD,            0  },
  {  "<VOLUME_DOWN>",         0,           0xAE,            0  },
  {  "<VOLUME_UP>",           0,           0xAF,            0  },
  {  "<MEDIA_NEXT_TRACK>",    0,           0xB0,            0  },
  {  "<MEDIA_PREV_TRACK>",    0,           0xB1,            0  },
  {  "<MEDIA_STOP>",          0,           0xB2,            0  },
  {  "<MEDIA_PLAY_PAUSE>",    0,           0xB3,            0  },
  {  "<LAUNCH_MAIL>",         0,           0xB4,            0  },
  {  "<LAUNCH_MEDIA_SELECT>", 0,           0xB5,            0  },
  {  "<LAUNCH_APP1>",         0,           0xB6,            0  },
  {  "<LAUNCH_APP2>",         0,           0xB7,            0  },
  {  "<OEM_1>",               0,           0xBA,            0  },
  {  "<OEM_2>",               0,           0xBF,            0  },
  {  "<OEM_3>",               0,           0xC0,            0  },
  {  "<OEM_4>",               0,           0xDB,            0  },
  {  "<OEM_5>",               0,           0xDC,            0  },
  {  "<OEM_6>",               0,           0xDD,            0  },
  {  "<OEM_7>",               0,           0xDE,            0  },
  {  "<OEM_8>",               0,           0xDF,            0  },
  {  "<OEM_102>",             0,           0xE2,            0  },
  {  "<IME_PROCESS>",         0,           0xE5,            0  },
  {  "<OEM_SPECIFIC>",        0,           0xE6,            0  },
  {  "<PACKET>",              0,           0xE7,            0  },
  {  "<ATTN>",                0,           0xF6,            0  },
  {  "<CRSEL>",               0,           0xF7,            0  },
  {  "<EXSEL>",               0,           0xF8,            0  },
  {  "<EREOF>",               0,           0xF9,            0  },
  {  "<PLAY>",                0,           0xFA,            0  },
  {  "<ZOOM>",                0,           0xFB,            0  },
  {  "<NONAME>",              0,           0xFC,            0  },
  {  "<PA1>",                 0,           0xFD,            0  },
  {  "<OEM_CLEA>",            0,           0xFE,            0  },
};

static const KeyDef UNKNOWN_KEY = { "<UNKNOWN>", 0, 0, 0 };

static const size_t key_table_len = sizeof(key_table) / sizeof(KeyDef);
static const size_t nokey_table_len = sizeof(nokey_table) / sizeof(KeyDef);
static const KeyDef *key_array[256];

void keys_init(void) {
  size_t i;
  int vk;
  for (i = 0; i < 256; i++) {
    key_array[i] = &UNKNOWN_KEY;
  }
  for (i = 0; i < nokey_table_len; i++) {
    vk = nokey_table[i].virt_code;
    if (vk > 0 && vk < 256) {
      key_array[vk] = &nokey_table[i];
    }
  }
  for (i = 0; i < key_table_len; i++) {
    vk = key_table[i].virt_code;
    if (vk > 0 && vk < 256) {
      key_array[vk] = &key_table[i];
    }
  }
}

const KeyDef *find_key_def_by_name(const char *name) {
  if (!name) return NULL;
  for (size_t i = 0; i < key_table_len; i++) {
    if (strcmp(key_table[i].name, name) == 0)
      return &key_table[i];
  }
  return NULL;
}

const KeyDef *find_key_def_by_scan_code(int code) {
  for (size_t i = 0; i < key_table_len; i++) {
    if (key_table[i].scan_code == code)
      return &key_table[i];
  }
  return NULL;
}

const KeyDef *find_key_def_by_virt_code(int code) {
  if (code < 0 || code > 255) return &UNKNOWN_KEY;
  return key_array[code];
}

int find_modifier_by_virt_code(int code) {
  if (code < 0 || code > 255) return 0;
  return key_array[code]->modifier;
}

const char *friendly_virt_code_name(int code) {
  return find_key_def_by_virt_code(code)->name;
}

