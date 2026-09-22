// Pushes the active layer number to the CONSOLE_ENABLE USB HID interface
// (a stock QMK feature, entirely separate from VIA/Vial's raw HID
// interface -- confirmed on hardware to keep working with vial.rocks open
// at the same time) as a plain text line, for a future desktop HUD app to
// read. See CLAUDE.md for why a separate interface matters here.
//
// Wire format (deliberately simple): "LAYER:<n>\n"

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
