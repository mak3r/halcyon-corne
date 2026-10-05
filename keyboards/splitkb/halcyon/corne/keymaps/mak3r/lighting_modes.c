// Lighting modes, cycled by RM_NEXT (layer 3, bottom-left) / RM_PREV.
//
// Every mode starts from the per-layer key colors in ledmap.c; modes differ
// in what they add on top (e.g. underglow). RM_NEXT/RM_PREV are intercepted
// here rather than cycling QMK's own RGB Matrix effects, for two reasons:
//   - the ledmap is painted in rgb_matrix_indicators_user(), which overwrites
//     whatever effect is running underneath, so stock effect cycling was
//     invisible anyway;
//   - implementing these as custom RGB Matrix effects instead would break
//     Vial's Lighting tab: VialRGB only knows stock effect IDs, reports an
//     unknown one as "off", and then sends "off" back as the mode on any
//     brightness slider change, disabling RGB entirely.
// So the RGB Matrix mode itself is left untouched (brightness/RM_TOGG/Vial
// keep working as before), and the selected lighting mode is our own state:
// persisted in the user EEPROM word, and synced to the slave half over
// RPC_ID_LIGHTING_MODE, since each half paints its own LEDs.
//
// Modes combine a recolor of the ledmap (an hsv_transform_t passed to
// set_layer_color()) with optional underglow; rgb_matrix_indicators_user()
// decodes the mode index into those two parts, so adding a mode means
// updating both the enum and that decoding.

#ifdef RGB_MATRIX_ENABLE

#    include "lighting_modes.h"
#    include "ledmap.h"
#    include "transactions.h"

// Two groups of three: the primary-color rotations first, then the same
// three again with purple underglow.
enum lighting_mode {
    LIGHTING_MODE_LAYERS,                  // per-layer key colors, underglow off
    LIGHTING_MODE_LAYERS_SHIFT_1,          // green->blue, blue->purple, purple->green
    LIGHTING_MODE_LAYERS_SHIFT_2,          // green->purple, blue->green, purple->blue
    LIGHTING_MODE_LAYERS_UNDERGLOW,        // the three above, plus purple underglow
    LIGHTING_MODE_LAYERS_SHIFT_1_UNDERGLOW,
    LIGHTING_MODE_LAYERS_SHIFT_2_UNDERGLOW,
    LIGHTING_MODE_COUNT,
};

static const HSV UNDERGLOW_PURPLE = {191, 255, 255}; // QMK's HSV_PURPLE

// The palette's three "primary" identifier colors, which the shift modes
// rotate. Matched by hue range rather than exact value so small palette
// tweaks in rgb_layers.csv still get rotated; saturation/value are kept, so
// e.g. the pale purple homing keys become pale green/blue. Everything
// outside these ranges (reds, oranges, the teal, greys/whites) stays as-is.
enum primary_color { PRIMARY_GREEN, PRIMARY_BLUE, PRIMARY_PURPLE, PRIMARY_COUNT, PRIMARY_NONE = PRIMARY_COUNT };

static const struct {
    uint8_t hue, min, max;
} PRIMARY_HUES[PRIMARY_COUNT] = {
    [PRIMARY_GREEN]  = {74, 60, 100},
    [PRIMARY_BLUE]   = {152, 140, 170},
    [PRIMARY_PURPLE] = {188, 175, 210},
};

static uint8_t primary_color_of(HSV hsv) {
    if (hsv.s == 0) {
        return PRIMARY_NONE; // grey/white -- hue is meaningless
    }
    for (uint8_t i = 0; i < PRIMARY_COUNT; i++) {
        if (hsv.h >= PRIMARY_HUES[i].min && hsv.h <= PRIMARY_HUES[i].max) {
            return i;
        }
    }
    return PRIMARY_NONE;
}

// Rotates the primaries forward by `steps` (green -> blue -> purple -> green).
static HSV shift_primaries(HSV hsv, uint8_t steps) {
    uint8_t primary = primary_color_of(hsv);
    if (primary != PRIMARY_NONE) {
        hsv.h = PRIMARY_HUES[(primary + steps) % PRIMARY_COUNT].hue;
    }
    return hsv;
}

static HSV shift_primaries_1(HSV hsv) {
    return shift_primaries(hsv, 1);
}

static HSV shift_primaries_2(HSV hsv) {
    return shift_primaries(hsv, 2);
}

static uint8_t lighting_mode        = LIGHTING_MODE_LAYERS;
static uint8_t lighting_mode_synced = LIGHTING_MODE_LAYERS; // slave's view, pushed from master

static void lighting_mode_sync_slave_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    if (in_buflen == sizeof(lighting_mode_synced)) {
        memcpy(&lighting_mode_synced, in_data, sizeof(lighting_mode_synced));
    }
}

static uint8_t lighting_mode_get_synced(void) {
    return is_keyboard_master() ? lighting_mode : lighting_mode_synced;
}

// Runs after quantum_init() has validated EEPROM, before either half starts
// exchanging transactions. keyboard_post_init_user() is already claimed by
// the shared caps_word_sync.c.
void matrix_init_user(void) {
    lighting_mode = eeconfig_read_user() & 0xFF;
    if (lighting_mode >= LIGHTING_MODE_COUNT) {
        lighting_mode = LIGHTING_MODE_LAYERS;
    }
    transaction_register_rpc(RPC_ID_LIGHTING_MODE, lighting_mode_sync_slave_handler);
}

static void lighting_mode_set(uint8_t mode) {
    lighting_mode = mode;
    eeconfig_update_user((eeconfig_read_user() & ~0xFFUL) | mode);
}

bool lighting_modes_process_record(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case RM_NEXT:
            if (record->event.pressed) {
                lighting_mode_set((lighting_mode + 1) % LIGHTING_MODE_COUNT);
            }
            return false;
        case RM_PREV:
            if (record->event.pressed) {
                lighting_mode_set((lighting_mode + LIGHTING_MODE_COUNT - 1) % LIGHTING_MODE_COUNT);
            }
            return false;
    }
    return true;
}

static void lighting_mode_sync_task(void) {
    static uint8_t  last_sent = 0xFF; // forces an initial send once the slave is up
    static uint32_t last_sync = 0;
    if (lighting_mode != last_sent && timer_elapsed32(last_sync) > 50) {
        if (transaction_rpc_send(RPC_ID_LIGHTING_MODE, sizeof(lighting_mode), &lighting_mode)) {
            last_sent = lighting_mode;
        }
        last_sync = timer_read32();
    }
}

static void set_underglow_color(HSV hsv) {
    RGB rgb = hsv_to_rgb_with_value(hsv);
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        if (HAS_FLAGS(g_led_config.flags[i], LED_FLAG_UNDERGLOW)) {
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        }
    }
}

bool rgb_matrix_indicators_user(void) {
    if (is_keyboard_master()) {
        lighting_mode_sync_task();
    }

    static const hsv_transform_t SHIFTS[] = {NULL, shift_primaries_1, shift_primaries_2};
    uint8_t                      mode     = lighting_mode_get_synced();

    set_layer_color(get_highest_layer(layer_state), SHIFTS[mode % 3]);
    if (mode >= LIGHTING_MODE_LAYERS_UNDERGLOW) {
        set_underglow_color(UNDERGLOW_PURPLE);
    }
    return true;
}

#endif
