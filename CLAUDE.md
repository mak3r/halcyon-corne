# CLAUDE.md — Agent Instructions

This is a solo hobby-hardware project. There is no multi-persona/worktree workflow here — just Claude working directly on `halcyon` (this repo's default branch, kept name-aligned with `upstream/halcyon` for easy syncing) with the human reviewing before anything gets pushed or tagged.

## Project Purpose

This is `mak3r/halcyon-corne`, a fork of [`splitkb/qmk_userspace`](https://github.com/splitkb/qmk_userspace) (the `halcyon` branch, which is Vial-enabled). It exists to build and release custom QMK/Vial firmware for a **Halcyon Corne rev2** — a wired split keyboard with a **TFT LCD display module on the left half** and a **Cirque trackpad module on the right half**, both attaching through splitkb's VIK module connector.

This repo is QMK/Vial + **wired only**. A wireless build of the same keyboard would use ZMK, which is a completely different toolchain (west/Zephyr, not `qmk compile`) with its own upstream repos (`splitkb/zmk-halcyon-config` + `splitkb/zmk-halcyon-module`). If that gets built later it becomes its own sibling repo (e.g. `halcyon-corne-zmk`), not a branch or subfolder here — there's no shared code or build tooling between QMK/Vial and ZMK.

Related repos/directories (not part of this repo, but relevant context):
- `~/projects/halcyon` — the splitkb Halcyon *case files* fork (3D-print/laser-cut files only, no firmware). Also holds:
  - `corne-vial/mak3rs.vil` — a live Vial-app keymap export with the actual intended key layout. This export uses the pre-refactor legacy row/column matrix (module buttons on extra rows), so it only imports cleanly onto firmware built with the `vial_hlc_legacy` keymap, not `vial_hlc` (which moved module buttons to extra columns) — see `README.md:98`. This isn't compiled firmware (Vial keymaps are remapped live, post-flash, without recompiling), but it's been baked directly into the `mak3r` keymap's source instead — see "The `mak3r` keymap" below.
  - `zsa_moonlander/keymap.c` (lines 91-128) — the source for the per-key/per-layer RGB `ledmap` + `set_layer_color()` pattern from a ZSA Moonlander/Oryx export, adapted (by key role, not literal LED index) for this keyboard's RGB colors — see "The `mak3r` keymap" below. LED colors, unlike the key layout, **do** require compiled firmware — Vial's UI has no per-key/per-layer color support.

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

## The `mak3r` keymap

`keyboards/splitkb/halcyon/corne/keymaps/mak3r/` is the custom keymap (forked from `vial_hlc_legacy`, for the same reason — compatibility with `mak3rs.vil`-style Vial exports). Unlike the stock keymaps, its key layout is baked into `keymap.json` directly (converted from a Vial export) rather than requiring a post-flash Vial import.

Per-key/per-layer RGB colors are **data-driven**, not hand-written C:
- `rgb_layers.csv` — the source of truth. One row per lit LED, keyed by `layer,row,col` (same 10×6 matrix coordinates as `keymap.json`), plus `h,s,v`. Supports `*` wildcards for row/col to fill a whole layer or row at once. Unlisted positions default to off.
- `generate_ledmap.py` — reads the CSV and emits `ledmap.c` (the actual `ledmap[][RGB_MATRIX_LED_COUNT][3]` array + `set_layer_color()`/`rgb_matrix_indicators_user()` hooks, following the pattern from `~/projects/halcyon/zsa_moonlander/keymap.c:91-128`, adapted by key role rather than literal LED index since the two boards' physical layouts differ).
- After editing `rgb_layers.csv`, run `python3 generate_ledmap.py` and commit the regenerated `ledmap.c` — the firmware build itself stays fully static, no Python involved at build/CI time.
- Only layer 0 (fully) and layer 1 (partially) have real color intent so far; layers 2-7 are a solid-blue placeholder pending further design.

This CSV-based approach is intentionally chosen to stay compatible with a possible future Oryx-like editor UI: both `keymap.json` (keycodes) and `rgb_layers.csv` (colors) are plain, structured formats a future tool could read/write directly, regenerating `ledmap.c` and triggering a rebuild — no architecture change needed to support that later.

Stock `vial_hlc_legacy` targets remain in `qmk.json` as the permanent known-good comparison.

**Keycode alias quirk**: the Vial app's `.vil` export uses long-form keycode names (`KC_LSHIFT`, `KC_BSPACE`, `KC_SCOLON`, `KC_RSHIFT`, `KC_LBRACKET`, `KC_RBRACKET`, `KC_BSLASH`, `KC_PGDOWN`, `KC_PSCREEN`, …) that this `vial-kb/vial-qmk` fork's keycode alias set doesn't recognize — they fail to compile with "undeclared" errors, even though other long aliases like `KC_ESCAPE`/`KC_ENTER` work fine. When converting a future Vial export into a keymap.json, cross-check every keycode against `data/constants/keycodes/*.hjson` in the vial-qmk checkout (or just try a build) and swap to the short-form alias (`KC_LSFT`, `KC_BSPC`, `KC_SCLN`, `KC_RSFT`, `KC_LBRC`, `KC_RBRC`, `KC_BSLS`, `KC_PGDN`, `KC_PSCR`) where needed.

**Local compile setup that actually works**: a plain `qmk setup` + Homebrew `arm-none-eabi-gcc` on this machine had a broken/incomplete toolchain bottle (missing newlib, `stdint.h` not found). What worked: `qmk setup -H <path> vial-kb/vial-qmk -b vial -y` to get the source tree (for reading real board files / grepping keycode data even without a working compiler), then compiling inside the same container CI uses — `docker run --rm -v <this repo>:/repo -v <vial-qmk checkout>:/qmk_firmware -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest sh -c 'qmk config user.qmk_home=/qmk_firmware; qmk config user.overlay_dir=/repo; qmk compile -kb splitkb/halcyon/corne/rev2 -km <keymap> -e <HLC_MODULE>=1 -e TARGET=<name>'` (symlink `converters/promicro_to_halcyon/` into `qmk_firmware/platforms/chibios/converters/` first, matching the CI workflow's prep step). This gives real compiler errors directly, unlike `gh run view --log-failed` on this repo's CI, which only shows terse `[ERRORS]`/`[OK]` summary lines with no detail (the actual failure detail goes to `$GITHUB_STEP_SUMMARY`, which isn't retrievable via the `gh` CLI or REST API). On this machine, Docker requires `podman machine start` first (this environment uses podman as the Docker backend), then `export DOCKER_HOST='unix:///var/folders/.../podman/podman-machine-default-api.sock'` (from that command's own output).

## Commit standards

- Conventional commit style: `<type>(<scope>): <description>` (`feat`, `fix`, `docs`, `ci`, `chore`, etc.)
- Include `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>` on commits made by Claude.
- Never reference a commit SHA in a comment/PR without verifying it with `git rev-parse --verify <sha>`.
