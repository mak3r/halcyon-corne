// Pushes the active layer number and individual keystrokes to the
// CONSOLE_ENABLE USB HID interface (a stock QMK feature, entirely separate
// from VIA/Vial's raw HID interface -- confirmed on hardware to keep
// working with vial.rocks open at the same time) as plain text lines, for
// the halcyon-corne-hud desktop app to read. See CLAUDE.md for why a
// separate interface matters here.
//
// Wire format (deliberately simple):
//   "LAYER:<n>\n"                  -- active layer changed
//   "KEY:<row>,<col>,<0|1>\n"      -- a key was released/pressed, by its
//                                     matrix position (same row/col space
//                                     as keymap.json and g_led_config)

#include QMK_KEYBOARD_H

layer_state_t layer_state_set_user(layer_state_t state) {
    static uint8_t last_layer = 255; // force a print on first call
    uint8_t        layer      = get_highest_layer(state);
    if (layer != last_layer) {
        last_layer = layer;
        uprintf("LAYER:%d\n", layer);
    }
    return state;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    uprintf("KEY:%d,%d,%d\n", record->event.key.row, record->event.key.col, record->event.pressed);
    return true;
}
