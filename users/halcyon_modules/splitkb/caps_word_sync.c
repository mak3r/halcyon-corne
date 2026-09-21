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
