// Per-layer key colors -- implemented in the generated ledmap.c (see
// generate_ledmap.py / rgb_layers.csv). The rgb_matrix_indicators_user()
// hook that calls these lives in lighting_modes.c.

#pragma once

#include QMK_KEYBOARD_H

// Optional per-LED color rewrite applied to every lit ledmap entry before
// it's painted (e.g. a hue rotation). NULL = paint the ledmap as-is.
typedef HSV (*hsv_transform_t)(HSV hsv);

// Paints every LED from the ledmap for `layer` (positions with no color,
// including the underglow LEDs, are set to off). Layers past the ledmap
// are painted all-off.
void set_layer_color(uint8_t layer, hsv_transform_t transform);

// HSV -> RGB, scaled by the live RGB Matrix brightness (RM_VALU/RM_VALD,
// Vial's Lighting tab), so every mode tracks the same brightness setting.
RGB hsv_to_rgb_with_value(HSV hsv);
