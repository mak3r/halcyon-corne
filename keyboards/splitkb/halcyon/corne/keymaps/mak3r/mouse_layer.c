// Layer 4 as the mouse layer: QMK's stock auto mouse layer
// (POINTING_DEVICE_AUTO_MOUSE_ENABLE, see config.h) turns layer 4 on as
// soon as the Cirque trackpad reports movement, and turns it back off after
// AUTO_MOUSE_TIME without mouse activity or on any non-mouse key. Layer 4
// still has TO(0) on both middle thumb keys as an explicit way out, and is
// still reachable manually via OSL(4)/TG(4) on layer 3.
//
// With SPLIT_POINTING_ENABLE + POINTING_DEVICE_COMBINED this runs on
// whichever half is master, off the combined report, so trackpad movement
// triggers it either way. The Cirque driver's extra touch-down-without-
// movement trigger only works when the right (Cirque) half is master.

#include "mouse_layer.h"

void pointing_device_init_user(void) {
    // Auto mouse is compiled in but starts disabled.
    set_auto_mouse_enable(true);
}

void mouse_layer_process_record(uint16_t keycode, keyrecord_t *record) {
    // TG(4) sets auto mouse's "toggled" flag, which keeps layer 4 latched
    // on. TO(0) moves back to layer 0 but auto mouse doesn't clear that
    // flag for it, so the next trackpad movement would latch layer 4 on
    // again with no timeout. Clear it here.
    if (record->event.pressed && IS_QK_TO(keycode) && QK_TO_GET_LAYER(keycode) == 0 && get_auto_mouse_toggle()) {
        auto_mouse_toggle();
    }
}
