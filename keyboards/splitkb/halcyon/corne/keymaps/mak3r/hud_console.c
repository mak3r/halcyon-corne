// PROTOTYPE -- not yet validated on hardware, not part of the mak3r
// keymap's real feature set. See experiment/console-hud branch.
//
// Pushes the active layer number to the CONSOLE_ENABLE USB HID interface
// (a stock QMK feature, entirely separate from VIA/Vial's raw HID
// interface -- see CLAUDE.md's split-state-sync notes for why that
// separation matters) as a plain text line, for a desktop HUD app to read.
//
// Wire format (deliberately trivial for a smoke test): "LAYER:<n>\n"

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
