# Desktop HUD

[`halcyon-corne-hud`](https://github.com/mak3r/halcyon-corne-hud) is a companion desktop app (a separate repo, not part of this one — see `CLAUDE.md`'s "Project Purpose" for why this repo stays scoped to firmware) that shows a real-time overlay of the active keymap layer, similar in spirit to ZSA's Keymapp. It's especially useful while learning a new layout: it highlights the key currently being held down, and can be pinned on-screen continuously instead of only flashing on layer changes.

![The desktop HUD overlay showing Layer 0 (Base), colored to match the keyboard's real per-key RGB](images/hud-overlay.png)

## How it connects to this firmware

The `mak3r` keymap's `hud_console.c` broadcasts two kinds of messages over QMK's `CONSOLE_ENABLE` USB HID interface — a stock, cross-platform QMK feature, deliberately a *separate* USB interface from VIA/Vial's own raw HID channel, so vial.rocks keeps working normally with the HUD app connected at the same time:

- `LAYER:<n>\n` — whenever the active layer changes (`layer_state_set_user()`)
- `KEY:<row>,<col>,<0|1>\n` — on every keypress/release, by matrix position (`process_record_user()`)

The desktop app listens on that interface and renders accordingly. See this repo's `CLAUDE.md` ("Desktop HUD layer + keystroke broadcast") for the wire format details, and the other repo's `CLAUDE.md`/README for the app side.

## Getting it

The HUD app has no distributed pre-built binary (by design — see that repo's README) — build it yourself from source:

```bash
git clone https://github.com/mak3r/halcyon-corne-hud
cd halcyon-corne-hud
# see its README's "Building and deploying (macOS)" section
```

It also needs its own copy of this repo's keymap/color data (regenerated from your `.vil` export and `rgb_layers.csv` — see that repo's `scripts/generate_layout_data.py`), and shares the same underlying color source the [Corne Palette Editor](PALETTE_EDITOR.md) writes to.

## Version compatibility

The two repos are tightly coupled — the HUD app can only show what the firmware actually broadcasts, so an older firmware build won't have data a newer HUD app expects. Check this table before pairing a specific release of each:

| `halcyon-corne-hud` | requires `halcyon-corne` | why |
|---|---|---|
| [v0.1.0](https://github.com/mak3r/halcyon-corne-hud/releases/tag/v0.1.0) | [v0.1.0-mak3r](https://github.com/mak3r/halcyon-corne/releases/tag/v0.1.0-mak3r) or later | `v0.1.0-mak3r` is the first firmware release broadcasting `KEY:` (per-keystroke) messages, which this HUD version relies on for its press-highlight feature — older `mak3r` builds only sent `LAYER:`. |

*(Keep this table's rows in sync with the identical copy in `halcyon-corne-hud`'s own `docs/HUD.md` whenever a new paired release goes out on either side.)*

Running a HUD app version against older firmware than its table row lists won't crash anything — unrecognized message types are just ignored — but any feature that depends on a newer broadcast (e.g. per-key highlighting) silently won't work until you reflash.
