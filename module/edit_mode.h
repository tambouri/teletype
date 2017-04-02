#ifndef _EDIT_MODE_H_
#define _EDIT_MODE_H_

#include "stdbool.h"
#include "stdint.h"

void set_edit_mode(void);
void process_edit_keys(uint8_t key, uint8_t mod_key, bool is_held_key);
void screen_refresh_edit(void);

#endif