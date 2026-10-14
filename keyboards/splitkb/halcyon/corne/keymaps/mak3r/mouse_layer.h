// Auto mouse layer glue -- see mouse_layer.c.

#pragma once

#include QMK_KEYBOARD_H

// Call from process_record_user() (hud_console.c owns that hook). Returns
// false when it has handled the key itself (the ACL speed keys).
bool mouse_layer_process_record(uint16_t keycode, keyrecord_t *record);
