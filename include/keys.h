#ifndef KEYS_H
#define KEYS_H

#define VIRT_CODE_SIZE 512 // power of 2
#define VIRT_CODE_MASK (VIRT_CODE_SIZE-1)

typedef struct {
  const char *name;
  int scan_code;
  int virt_code;
  int modifier;
} KeyDef;

void keys_init(void);
int find_modifier_by_virt_code(int code);
const KeyDef *find_key_def_by_name(const char *name);
//const KeyDef *find_key_def_by_scan_code(int code);
//const KeyDef *find_key_def_by_virt_code(int code);
const char *friendly_virt_code_name(int code);

#endif // KEYS_H
