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
#include "transactions.h"

// The Cirque module sits in the right half's VIK slot with a slight
// north/south tilt -- on hardware, dragging straight "north" (away from
// the user) reads as moving up and slightly northwest instead. This
// rotates the right half's raw x/y by a fixed angle to correct for it,
// before anything else (cursor movement, drag-to-scroll's axis lock)
// consumes the report. Positive degrees rotate the corrected direction
// clockwise; if up still drifts to one side, or drifts the other way,
// flip the sign (recompute the two constants below for the new angle --
// see docs/TRACKPAD.md). Precomputed rather than calling sinf/cosf at
// runtime, since the angle is fixed at build time.
//
// 8 degrees: cos(8 deg) = 0.9902681, sin(8 deg) = 0.1391731
#define CIRQUE_TILT_COS 0.9902681f
#define CIRQUE_TILT_SIN 0.1391731f

static int8_t clamp_i8(float v) {
    if (v > 127.0f) return 127;
    if (v < -127.0f) return -127;
    return (int8_t)v;
}

static void apply_cirque_tilt_correction(report_mouse_t *report) {
    float x = report->x;
    float y = report->y;
    report->x = clamp_i8(x * CIRQUE_TILT_COS - y * CIRQUE_TILT_SIN);
    report->y = clamp_i8(x * CIRQUE_TILT_SIN + y * CIRQUE_TILT_COS);
}

#if defined(SCROLL_DRAG_MODE_ENABLE)
// Toggle-able drag-to-scroll mode: A key, layer 4 (left of RClk/S) turns
// trackpad drags into scroll events instead of cursor movement, as an
// alternative to the circular edge-scroll gesture. See docs/TRACKPAD.md.

static bool scroll_drag_active        = false;
static bool scroll_drag_synced_active = false; // slave's view, pushed from master below
static bool scroll_drag_added_latch   = false; // did WE set auto mouse's toggle flag -- see mouse_layer_process_record()

static void scroll_drag_sync_slave_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    if (in_buflen == sizeof(bool)) {
        memcpy(&scroll_drag_synced_active, in_data, sizeof(bool));
    }
}

// What hlc_tft_display.c actually calls: scroll_drag_active directly on the
// master, the synced copy on the slave -- same pattern as
// is_caps_word_active_synced() in caps_word_sync.c.
bool is_scroll_drag_active_synced(void) {
    return is_keyboard_master() ? scroll_drag_active : scroll_drag_synced_active;
}

// Raw trackpad units per emitted scroll "click" -- report_mouse_t's v/h
// fields are wheel-click counts (like a real scroll wheel's detents), not
// pixels, so passing raw x/y straight through would scroll wildly too
// fast. Smaller = faster scrolling. Tune by feel, same spirit as
// cirque_pinnacle_configure_circular_scroll()'s wheel_clicks parameter --
// see docs/TRACKPAD.md. (Started at 8, then 32; still too fast on
// hardware, now 40. Not required to be a power of 2 -- it's just an
// integer divisor.)
#    define SCROLL_DRAG_THRESHOLD 40

// How long with zero motion before the next movement is allowed to pick a
// new axis -- long enough that a brief natural pause mid-drag doesn't
// "forget" the locked axis, short enough that lifting your finger and
// starting a fresh drag (even quickly) reliably re-picks it. This is the
// answer to "do I need to toggle off/on to switch between horizontal and
// vertical": no -- lift and redrag in the other direction instead.
#    define SCROLL_DRAG_AXIS_RESET_MS 150

typedef enum { SCROLL_AXIS_NONE, SCROLL_AXIS_H, SCROLL_AXIS_V } scroll_axis_t;

static void apply_scroll_drag(report_mouse_t *report) {
    static scroll_axis_t axis           = SCROLL_AXIS_NONE;
    static int16_t       accum          = 0;
    static uint32_t      last_motion_at = 0;

    if (report->x == 0 && report->y == 0) {
        if (timer_elapsed32(last_motion_at) > SCROLL_DRAG_AXIS_RESET_MS) {
            axis = SCROLL_AXIS_NONE; // finger lifted (or idle long enough) -- next motion re-picks the axis
        }
        return;
    }

    last_motion_at = timer_read32();
    if (axis == SCROLL_AXIS_NONE) {
        axis = (abs(report->x) >= abs(report->y)) ? SCROLL_AXIS_H : SCROLL_AXIS_V;
    }

    accum += (axis == SCROLL_AXIS_H) ? report->x : report->y;
    int8_t clicks = accum / SCROLL_DRAG_THRESHOLD;
    accum -= (int16_t)clicks * SCROLL_DRAG_THRESHOLD;

    report->x = 0;
    report->y = 0;
    report->h = (axis == SCROLL_AXIS_H) ? clicks : 0;
    report->v = (axis == SCROLL_AXIS_V) ? -clicks : 0; // natural-scroll sign; flip if it feels backwards
}
#endif

void pointing_device_init_user(void) {
    // Auto mouse is compiled in but starts disabled.
    set_auto_mouse_enable(true);
#if defined(SCROLL_DRAG_MODE_ENABLE)
    transaction_register_rpc(RPC_ID_SCROLL_DRAG, scroll_drag_sync_slave_handler);
#endif
}

void mouse_layer_process_record(uint16_t keycode, keyrecord_t *record) {
    // TG(4) sets auto mouse's "toggled" flag, which keeps layer 4 latched
    // on. TO(0) moves back to layer 0 but auto mouse doesn't clear that
    // flag for it, so the next trackpad movement would latch layer 4 on
    // again with no timeout. Clear it here.
    if (record->event.pressed && IS_QK_TO(keycode) && QK_TO_GET_LAYER(keycode) == 0 && get_auto_mouse_toggle()) {
        auto_mouse_toggle();
    }

#if defined(SCROLL_DRAG_MODE_ENABLE)
    // Reusing KC_F13 as an internal-only trigger rather than a real custom
    // keycode, so this stays fully editable in Vial/vial.rocks with no
    // customKeycodes setup needed. The real F13 press/release still
    // reaches the host too; harmless, nothing binds F13 to anything by
    // default. Toggle (not hold): tap once to engage, tap again to
    // disengage -- a hold-style key would need the same hand to hold both
    // the layer-4 trigger and this key at once, awkward when that hand is
    // also meant to be free for the trackpad itself.
    if (keycode == KC_F13 && record->event.pressed) {
        scroll_drag_active = !scroll_drag_active;

        // This key only exists on layer 4's keymap grid, so pressing it at
        // all means layer 4 is active right now -- but auto mouse's own
        // AUTO_MOUSE_TIME (650ms) idle timeout would otherwise still turn
        // layer 4 back off on its own schedule. If that happened between
        // toggling scroll-drag on and tapping this key again to toggle it
        // off, the second tap would land on whatever layer 0 has at this
        // position (KC_A) instead of reaching this handler at all --
        // toggling on would appear to work but toggling off wouldn't.
        // Latch layer 4 on for as long as scroll-drag is active, the same
        // mechanism TG(4) already uses (see the TO(0) handling above).
        // Track whether WE did the latching so we don't clobber the
        // user's own independent TG(4) toggle if they'd already set it.
        if (scroll_drag_active) {
            if (!get_auto_mouse_toggle()) {
                auto_mouse_toggle();
                scroll_drag_added_latch = true;
            }
        } else if (scroll_drag_added_latch) {
            if (get_auto_mouse_toggle()) {
                auto_mouse_toggle();
            }
            scroll_drag_added_latch = false;
        }
    }
#endif
}

// Diagnostic logging below (MOTION:) predates and is unrelated to scroll-
// drag mode above -- investigating a real-hardware report of the RGB LEDs
// failing to time out overnight on one half; see CLAUDE.md/issue #1. This
// function itself is no longer gated on CONSOLE_ENABLE (only the logging
// inside it is), since scroll-drag's own transform needs to run
// regardless of whether the diagnostic channel is compiled in.
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    apply_cirque_tilt_correction(&right_report);

#if defined(CONSOLE_ENABLE)
    if (left_report.x || left_report.y || right_report.x || right_report.y) {
        static uint32_t last_log = 0;
        if (timer_elapsed32(last_log) > 100) {
            uprintf("MOTION:%lu,%d,%d,%d,%d\n", timer_read32(), (int)left_report.x, (int)left_report.y, (int)right_report.x, (int)right_report.y);
            last_log = timer_read32();
        }
    }
#endif

#if defined(SCROLL_DRAG_MODE_ENABLE)
    if (scroll_drag_active) {
        apply_scroll_drag(&left_report);
        apply_scroll_drag(&right_report);
    }

    if (is_keyboard_master()) {
        static bool     last_sent = false;
        static uint32_t last_sync = 0;
        if (scroll_drag_active != last_sent && timer_elapsed32(last_sync) > 50) {
            if (transaction_rpc_send(RPC_ID_SCROLL_DRAG, sizeof(scroll_drag_active), &scroll_drag_active)) {
                last_sent = scroll_drag_active;
            }
            last_sync = timer_read32();
        }
    }
#endif

    return pointing_device_combine_reports(left_report, right_report);
}
