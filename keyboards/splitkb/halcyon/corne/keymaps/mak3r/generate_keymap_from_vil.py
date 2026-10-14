#!/usr/bin/env python3
"""Regenerate keymap.json from a Vial keymap export (.vil).

Usage:
    python3 generate_keymap_from_vil.py path/to/export.vil

Run this whenever the keymap changes in the Vial app: export a fresh .vil
from Vial, point this script at it, and commit the regenerated keymap.json.
It does NOT touch rgb_layers.csv / ledmap.c -- regenerate those separately
with generate_ledmap.py if the key layout change also needs new colors.

Requires the .vil to have been exported under the LEGACY row/column matrix
(module buttons as extra rows -- see CLAUDE.md's "Related repos" section on
mak3rs.vil), matching this keymap's HALCYON_LEGACY / LAYOUT_corne_hlc setup.

keymap.json is the layout shared with the wireless Typeractive Corne
(tya-corne), so it can hold Bluetooth/output keycodes that QMK and Vial
don't have (WIRELESS_ONLY_KEYCODES, #defined to KC_NO in config.h). Vial
exports show those spots as KC_NO, so a spot where keymap.json has one of
these and the .vil has KC_NO keeps the keymap.json keycode.
"""
import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).parent
KEYMAP_JSON_PATH = HERE / "keymap.json"

HEADER_COMMENT = """// There is an extra row added for the Halcyon modules. Currently only the Encoder module is
// supported but we reserve 5 keys per half for future expansion. Your personal keymap will also
// need to be updated to include this row, and the `LAYOUT` macro will need to be updated to
// `LAYOUT_corne_hlc` in order to compile.
"""

# This vial-qmk fork's keycode alias set drops a handful of long-form
# aliases that Vial's .vil export uses -- see CLAUDE.md's "Keycode alias
# quirk" note. Add to this table if a future export hits a new one (the
# build will fail with "'KC_X' undeclared ... did you mean 'KC_Y'").
KEYCODE_ALIAS_FIXUPS = {
    "KC_LSHIFT": "KC_LSFT",
    "KC_RSHIFT": "KC_RSFT",
    "KC_BSPACE": "KC_BSPC",
    "KC_SCOLON": "KC_SCLN",
    "KC_LBRACKET": "KC_LBRC",
    "KC_RBRACKET": "KC_RBRC",
    "KC_BSLASH": "KC_BSLS",
    "KC_PGDOWN": "KC_PGDN",
    "KC_PSCREEN": "KC_PSCR",
    # Vial's short mouse key names. These compile, but keymap.json, the HUD
    # and tya-corne's generator all use the long spelling.
    "KC_MS_U": "KC_MS_UP",
    "KC_MS_D": "KC_MS_DOWN",
    "KC_MS_L": "KC_MS_LEFT",
    "KC_MS_R": "KC_MS_RIGHT",
    "KC_WH_U": "KC_MS_WH_UP",
    "KC_WH_D": "KC_MS_WH_DOWN",
    "KC_WH_L": "KC_MS_WH_LEFT",
    "KC_WH_R": "KC_MS_WH_RIGHT",
}


# Keycodes in the shared layout that only the Typeractive (ZMK) can do.
# Must match the #defines in config.h and tya-corne's generate_keymap.py.
WIRELESS_ONLY_KEYCODES = {
    "OUT_TOG", "BT_CLR", "BT_SEL_0", "BT_SEL_1", "BT_SEL_2", "BT_SEL_3", "BT_SEL_4",
}


_ALIAS_PATTERN = re.compile(r"\b(" + "|".join(KEYCODE_ALIAS_FIXUPS) + r")\b")


def fix_keycode(kc):
    """Substring-replace known bad aliases, since a keycode can appear
    wrapped in a modifier function, e.g. LSFT(KC_LBRACKET)."""
    return _ALIAS_PATTERN.sub(lambda m: KEYCODE_ALIAS_FIXUPS[m.group(1)], kc)


def flatten(grid):
    """Reshape a vil layer's 10x6 row-major grid into LAYOUT_corne_hlc's
    52-arg order: left rows in ascending column order, right rows in
    descending column order, except the module rows (4/9) which stay
    ascending on both halves. See CLAUDE.md for the derivation."""
    out = []
    out += grid[0][0:6]
    out += list(reversed(grid[5]))
    out += grid[1][0:6]
    out += list(reversed(grid[6]))
    out += grid[2][0:6]
    out += list(reversed(grid[7]))
    out += grid[3][3:6]
    out += list(reversed(grid[8]))[0:3]
    out += grid[4][0:5]
    out += grid[9][0:5]
    assert len(out) == 52, f"expected 52 keys, got {len(out)}"
    for v in out:
        assert v != -1, "hole leaked into flattened output -- unexpected vil matrix shape"
    return [fix_keycode(v) for v in out]


def fmt_row(vals, width=10):
    return ", ".join(f'"{v}"'.ljust(width) for v in vals)


def load_existing_layers():
    """The current keymap.json's layers (52-entry flat lists), or None."""
    if not KEYMAP_JSON_PATH.exists():
        return None
    text = re.sub(r"^\s*//.*$", "", KEYMAP_JSON_PATH.read_text(), flags=re.M)
    return json.loads(text)["layers"]


def keep_wireless_keycodes(flat_layers, existing):
    """Put back wireless-only keycodes that the .vil can only show as KC_NO."""
    if existing is None:
        return
    for li, (new, old) in enumerate(zip(flat_layers, existing)):
        for pos, (new_kc, old_kc) in enumerate(zip(new, old)):
            if old_kc in WIRELESS_ONLY_KEYCODES:
                if new_kc == "KC_NO":
                    new[pos] = old_kc
                else:
                    print(f"note: layer {li} pos {pos}: {old_kc} replaced by {new_kc} from the .vil")


def build_keymap_json(vil, keymap_name):
    layers = vil["layout"]
    encoders = vil["encoder_layout"]
    assert len(layers) == 8, f"expected 8 layers (DYNAMIC_KEYMAP_LAYER_COUNT), got {len(layers)}"

    flat_layers = [flatten(layer) for layer in layers]
    keep_wireless_keycodes(flat_layers, load_existing_layers())

    layer_blocks = []
    for f in flat_layers:
        line0 = fmt_row(f[0:12])
        line1 = fmt_row(f[12:24])
        line2 = fmt_row(f[24:36])
        line3 = fmt_row(f[36:42])
        line4 = fmt_row(f[42:52])
        block = (
            "        [\n"
            f"            {line0} ,\n"
            f"            {line1} ,\n"
            f"            {line2} ,\n"
            f"                                                {line3} ,\n"
            f"                        {line4}\n"
            "        ]"
        )
        layer_blocks.append(block)
    layers_json = ",\n".join(layer_blocks)

    enc_layer_blocks = []
    for enc_layer in encoders:
        parts = ", ".join(
            f'{{"ccw": "{fix_keycode(ccw)}", "cw": "{fix_keycode(cw)}"}}' for ccw, cw in enc_layer
        )
        enc_layer_blocks.append(f"        [{parts}]")
    encoders_json = ",\n".join(enc_layer_blocks)

    return f"""{HEADER_COMMENT}
{{
    "keyboard": "splitkb/halcyon/corne/rev2",
    "keymap": "{keymap_name}",
    "version": 1,
    "layout": "LAYOUT_corne_hlc",
    "layers": [
{layers_json}
    ],
    "encoders": [
{encoders_json}
    ]
}}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vil_path", type=Path, help="Path to the exported .vil file")
    parser.add_argument("--keymap-name", default="mak3r")
    args = parser.parse_args()

    vil = json.loads(args.vil_path.read_text())
    output = build_keymap_json(vil, args.keymap_name)
    KEYMAP_JSON_PATH.write_text(output)
    print(f"wrote {KEYMAP_JSON_PATH}")


if __name__ == "__main__":
    main()
