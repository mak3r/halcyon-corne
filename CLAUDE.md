# CLAUDE.md — Agent Instructions

This is a solo hobby-hardware project. There is no multi-persona/worktree workflow here — just Claude working directly on `halcyon` (this repo's default branch, kept name-aligned with `upstream/halcyon` for easy syncing) with the human reviewing before anything gets pushed or tagged.

## Project Purpose

This is `mak3r/halcyon-corne`, a fork of [`splitkb/qmk_userspace`](https://github.com/splitkb/qmk_userspace) (the `halcyon` branch, which is Vial-enabled). It exists to build and release custom QMK/Vial firmware for a **Halcyon Corne rev2** — a wired split keyboard with a **TFT LCD display module on the left half** and a **Cirque trackpad module on the right half**, both attaching through splitkb's VIK module connector.

This repo is QMK/Vial + **wired only**. A wireless build of the same keyboard would use ZMK, which is a completely different toolchain (west/Zephyr, not `qmk compile`) with its own upstream repos (`splitkb/zmk-halcyon-config` + `splitkb/zmk-halcyon-module`). If that gets built later it becomes its own sibling repo (e.g. `halcyon-corne-zmk`), not a branch or subfolder here — there's no shared code or build tooling between QMK/Vial and ZMK.

Related repos/directories (not part of this repo, but relevant context):
- `~/projects/halcyon` — the splitkb Halcyon *case files* fork (3D-print/laser-cut files only, no firmware). Also holds:
  - `corne-vial/mak3rs.vil` — a live Vial-app keymap export with the actual intended key layout. This isn't compiled firmware (Vial keymaps are remapped live, post-flash, without recompiling), so it doesn't need to be ported into source here — just re-imported into the Vial app after flashing.
  - `zsa_moonlander/keymap.c` (lines 91-128) — the source for the per-key/per-layer RGB `ledmap` + `set_layer_color()` pattern from a ZSA Moonlander/Oryx export, which is the reference for the LED-color work planned for this keyboard (see "Planned work" below). LED colors, unlike the key layout, **do** require compiled firmware — Vial's UI has no per-key/per-layer color support.

## How this repo works (inherited from splitkb/qmk_userspace)

- **`qmk.json`** is the source of truth for what gets built: a flat list of `[keyboard, keymap, {env vars incl. TARGET}]` tuples. Add/remove targets with `qmk userspace-add` / `qmk userspace-remove`, or edit the JSON directly.
- **Module selection is a build-time flag, not code.** Which module is on a given half is chosen per target via an `-e HLC_<MODULE>=1` env var (`HLC_TFT_DISPLAY`, `HLC_CIRQUE_TRACKPAD`, `HLC_ENCODER`, `HLC_ENCODER_REV2`, `HLC_NONE`). The actual driver code for each module already lives in `users/halcyon_modules/splitkb/` — don't duplicate or fork that logic into a keymap; hook into it via the documented `module_post_init_user()` / `display_module_housekeeping_task_user()` etc. functions instead (see `docs/MODULES.md`).
- **`TARGET` naming convention**: `halcyon_corne_<connectivity>_<side>_<module>` (e.g. `halcyon_corne_wired_left_tftdisplay`). Always wired for now — the connectivity segment exists so a future variant doesn't require renaming existing targets.
- **CI is already wired up** (`.github/workflows/build_binaries.yaml`) — every push builds all `qmk.json` targets against `vial-kb/vial-qmk` and publishes them to a GitHub Release. No changes needed there for ordinary keymap/module work.
- **Release model**: the CI-driven `latest-halcyon-vial` release is a **rolling** release — it's overwritten on every push, so it is not a safe rollback point. Cut an explicit, immutable, versioned tag + release (e.g. `v0.0.1-stock`) whenever there's a build worth being able to get back to. `git tag`/`gh release create` with copies of that build's `.uf2` assets — see `v0.0.1-stock` for the pattern.

## Local build/setup

```bash
qmk config user.overlay_dir="$(realpath .)"
qmk userspace-compile          # build every target in qmk.json
qmk userspace-compile -n       # print the compile commands without running them
qmk compile -kb splitkb/halcyon/corne/rev2 -km <keymap> -e <HLC_MODULE>=1 -e TARGET=<name>
```
Requires a local QMK CLI + a `vial-kb/vial-qmk` checkout set up via `qmk setup -H path/to/vial-qmk` (see upstream README for full first-time setup). CI does not require any of this locally — pushing is sufficient to get a build.

## Planned work (not yet done)

1. Copy the stock `vial_hlc` keymap to a `mak3r` keymap, add matching `qmk.json` targets, and keep the stock targets as a permanent known-good comparison.
2. Port the `ledmap`/`set_layer_color()` pattern from `~/projects/halcyon/zsa_moonlander/keymap.c:91-128` into the new keymap, sized off `RGB_MATRIX_LED_COUNT` and this board's own `keyboards/splitkb/halcyon/corne/rev2/keyboard.json` LED layout (54 LEDs, 27/side) rather than hardcoded Moonlander LED indices.

## Commit standards

- Conventional commit style: `<type>(<scope>): <description>` (`feat`, `fix`, `docs`, `ci`, `chore`, etc.)
- Include `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>` on commits made by Claude.
- Never reference a commit SHA in a comment/PR without verifying it with `git rev-parse --verify <sha>`.
