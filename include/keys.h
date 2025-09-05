#ifndef KEYS_H
#define KEYS_H

typedef struct {
  const char *name;
  int scan_code;
  int virt_code;
  int modifier;
} KeyDef;

#define MOUSE_DUMMY_VK 0xFF

void keys_init(void);
int find_modifier_by_virt_code(int code);
const KeyDef *find_key_def_by_name(const char *name);
//const KeyDef *find_key_def_by_scan_code(int code);
//const KeyDef *find_key_def_by_virt_code(int code);
const char *friendly_virt_code_name(int code);

#endif // KEYS_H
