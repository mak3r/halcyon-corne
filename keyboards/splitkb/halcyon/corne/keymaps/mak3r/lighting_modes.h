// Lighting modes cycled by RM_NEXT -- see lighting_modes.c.

#pragma once

#include QMK_KEYBOARD_H

// Call from process_record_user() (hud_console.c owns that hook). Returns
// false when it consumed the keycode.
bool lighting_modes_process_record(uint16_t keycode, keyrecord_t *record);
