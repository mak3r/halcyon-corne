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

#if defined(CONSOLE_ENABLE)
// Diagnostic: log every non-zero raw pointing-device report, rate-limited
// to 10/sec. Investigating a real-hardware report of the RGB LEDs failing
// to time out overnight on one half (the Cirque/right half specifically,
// which was also split master at the time) while the other half correctly
// went dark. Any pointing-device motion -- even a tiny spurious blip from
// environmental noise/static on the Cirque sensor -- updates
// last_input_activity_time via last_pointing_device_activity_trigger()
// (quantum/keyboard.c), which is exactly what RGB_MATRIX_TIMEOUT checks.
// This log is how we find out whether that's actually happening: run
// hud_console_test.py (or the real HUD app) overnight and check the
// morning's log for MOTION lines with no corresponding real trackpad use.
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    if (left_report.x || left_report.y || right_report.x || right_report.y) {
        static uint32_t last_log = 0;
        if (timer_elapsed32(last_log) > 100) {
            uprintf("MOTION:%lu,%d,%d,%d,%d\n", timer_read32(), (int)left_report.x, (int)left_report.y, (int)right_report.x, (int)right_report.y);
            last_log = timer_read32();
        }
    }
    return pointing_device_combine_reports(left_report, right_report);
}
#endif
