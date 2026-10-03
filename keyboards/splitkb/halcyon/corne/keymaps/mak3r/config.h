/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2024 splitkb.com <support@splitkb.com> */

#pragma once

#define VIAL_KEYBOARD_UID {0xF8, 0x7A, 0x1D, 0x23, 0x53, 0x9B, 0x54, 0xB9}

#define VIAL_UNLOCK_COMBO_ROWS { 0, 5 }
#define VIAL_UNLOCK_COMBO_COLS { 5, 5 }

#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_KEYPRESSES

// Default RGB Matrix brightness for a fresh/reset EEPROM (0-255). Only
// applies on first init or after an EEPROM reset -- a normal reflash keeps
// whatever brightness is already saved. Adjust live via Vial's Lighting tab.
#define RGB_MATRIX_DEFAULT_VAL 20

// Disabled for now (0 = off, per QMK's `#if RGB_MATRIX_TIMEOUT > 0` guard --
// compiles the timeout branch out entirely, not just a runtime no-op) --
// was intended to turn the key LEDs off after 15 minutes idle, but one half
// intermittently fails to actually turn off when this is enabled. See
// https://github.com/mak3r/halcyon-corne/issues/1 before re-enabling.
// The TFT backlight has its own, separate, shorter timeout
// (HLC_BACKLIGHT_TIMEOUT) which is unaffected by this.
#define RGB_MATRIX_TIMEOUT 0
// Share the input-activity timestamp across the split link. Kept enabled
// even with the timeout above disabled -- the diagnostic ACT: broadcast in
// caps_word_sync.c (see issue #1) reads the same last_*_activity_elapsed()
// functions this feeds, so turning it off would blind that instrumentation
// for whenever the investigation resumes.
#define SPLIT_ACTIVITY_ENABLE

#define DYNAMIC_KEYMAP_LAYER_COUNT 8

// Layer 4 is the mouse layer: trackpad movement turns it on automatically
// (see mouse_layer.c). AUTO_MOUSE_TIME (default 650ms) is how long it stays
// on after the last mouse activity -- the knob to tune if it drops out too
// quickly or lingers.
#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER 4

// Toggle-able drag-to-scroll mode (A key, layer 4 -- see mouse_layer.c).
// Guards hlc_tft_display.c's "Scroll" indicator so it also lights up for
// this, not just real Scroll Lock -- same pattern as CAPS_WORD_ENABLE's
// guard there, but this one is mak3r-only (not a repo-wide default), since
// the feature only exists in this keymap's own mouse_layer.c.
#define SCROLL_DRAG_MODE_ENABLE

#define HALCYON_LEGACY

#undef MATRIX_ROWS
#define MATRIX_ROWS 10
#define LAYOUT_corne_hlc(k0A, k0B, k0C, k0D, k0E, k0F, k5F, k5E, k5D, k5C, k5B, k5A, k1A, k1B, k1C, k1D, k1E, k1F, k6F, k6E, k6D, k6C, k6B, k6A, k2A, k2B, k2C, k2D, k2E, k2F, k7F, k7E, k7D, k7C, k7B, k7A, k3D, k3E, k3F, k8F, k8E, k8D, k4A, k4B, k4C, k4D, k4E, k9A, k9B, k9C, k9D, k9E) { \
    {k0A, k0B, k0C, k0D, k0E, k0F}, \
    {k1A, k1B, k1C, k1D, k1E, k1F}, \
    {k2A, k2B, k2C, k2D, k2E, k2F}, \
    {KC_NO, KC_NO, KC_NO, k3D, k3E, k3F}, \
    {k4A, k4B, k4C, k4D, k4E, KC_NO}, \
    {k5A, k5B, k5C, k5D, k5E, k5F}, \
    {k6A, k6B, k6C, k6D, k6E, k6F}, \
    {k7A, k7B, k7C, k7D, k7E, k7F}, \
    {KC_NO, KC_NO, KC_NO, k8D, k8E, k8F}, \
    {k9A, k9B, k9C, k9D, k9E, KC_NO} \
}
