// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Syncs Caps Word state from the split master to the slave. QMK has no
// built-in sync for this (unlike layer_state/host LED state, see
// SPLIT_LAYER_STATE_ENABLE/SPLIT_LED_STATE_ENABLE in this folder's
// config.h), so on its own is_caps_word_on() only reflects reality on
// whichever half is currently master -- a module like the TFT display can
// end up on either physical half depending on which side is plugged into
// USB. CAPS_WORD_ENABLE defaults to "yes" for every Vial keymap
// (build_vial.mk), so this compiles unconditionally for every keymap here,
// guarded in case a keymap ever explicitly turns it off.

#ifdef CAPS_WORD_ENABLE

#include QMK_KEYBOARD_H
#include "caps_word.h"
#include "transactions.h"
#include "split_util.h"

static bool caps_word_synced_active = false;

static void caps_word_sync_slave_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    if (in_buflen == sizeof(bool)) {
        memcpy(&caps_word_synced_active, in_data, sizeof(bool));
    }
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(RPC_ID_CAPS_WORD, caps_word_sync_slave_handler);
}

void housekeeping_task_user(void) {
    // Unrelated to Caps Word -- this is the shared repo-wide
    // housekeeping_task_user() hook, already claimed here, so a periodic
    // diagnostic build-version broadcast (for ruling out a firmware
    // mismatch between the two physical halves, e.g. after only one side
    // got reflashed) lives here too rather than fighting over the hook.
    // Periodic, not just once at boot, so it's visible to a listener
    // connecting at any time, not just within a race window right after
    // power-up -- the corne-kbd-hud app surfaces this in its tray tooltip.
    // Fires on both halves (not gated to master) so whichever half you
    // swap to be master shows ITS OWN build info. No-op (compiles out
    // entirely) on keymaps without CONSOLE_ENABLE.
    //
    // __DATE__/__TIME__ reflect whatever system clock/timezone the
    // compiler ran under -- confirmed this differs from the user's local
    // time when building via the documented Docker workflow (the
    // container defaults to UTC regardless of host timezone), and CI
    // (GitHub Actions) also runs in UTC by default. Rather than chasing
    // TZ passthrough separately for every build environment this repo
    // might compile in (local Docker, a native toolchain, CI), the
    // broadcast just says explicitly what it is -- correct everywhere,
    // no environment-specific fixup needed.
#if defined(CONSOLE_ENABLE)
    static uint32_t last_build_broadcast = 0;
    if (timer_elapsed32(last_build_broadcast) > 10000) {
        uprintf("BUILD:%s %s UTC\n", __DATE__, __TIME__);
        // Investigating RGB_MATRIX_TIMEOUT not firing on the master half
        // despite zero logged MOTION/KEY events overnight (see CLAUDE.md's
        // "Open investigation") -- this logs the actual elapsed-since-
        // activity values rgb_matrix_task() itself checks
        // (last_input_activity_elapsed() > RGB_MATRIX_TIMEOUT is the ONLY
        // non-suspend gate it uses), broken down by category, so we can
        // see directly whether the timer is genuinely growing past 900000
        // (pointing to a bug in how the LEDs respond to that, not in
        // activity tracking) or quietly resetting (pointing to an
        // activity source this investigation hasn't found yet).
        uprintf("ACT:%lu,%lu,%lu,%lu,%d\n", (unsigned long)last_input_activity_elapsed(), (unsigned long)last_matrix_activity_elapsed(), (unsigned long)last_encoder_activity_elapsed(), (unsigned long)last_pointing_device_activity_elapsed(), is_keyboard_master());
        last_build_broadcast = timer_read32();
    }
#endif

    if (is_keyboard_master()) {
        static bool last_sent = false;
        static uint32_t last_sync = 0;
        bool active = is_caps_word_on();
        if (active != last_sent && timer_elapsed32(last_sync) > 50) {
            if (transaction_rpc_send(RPC_ID_CAPS_WORD, sizeof(active), &active)) {
                last_sent = active;
            }
            last_sync = timer_read32();
        }
    }
}

// What hlc_tft_display.c actually calls: is_caps_word_on() directly on the
// master, the synced copy on the slave.
bool is_caps_word_active_synced(void) {
    return is_keyboard_master() ? is_caps_word_on() : caps_word_synced_active;
}

#endif // CAPS_WORD_ENABLE
