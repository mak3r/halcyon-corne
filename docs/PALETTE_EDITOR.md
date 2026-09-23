# Corne Palette Editor

Per-key/per-layer RGB colors aren't editable anywhere in Vial's UI — VialRGB's own UI only controls global brightness/animation mode, not individual key colors per layer. This fork has its own tool for that instead: the **Corne Palette Editor**, an interactive picker matching the Corne's actual physical layout (column stagger, thumb clusters, both halves), showing each key's real current keycode for context so you're coloring against the layout you actually have, not a generic grid.

![Corne Palette Editor, showing layer 0 with per-key colors and the color-picker panel](images/palette-editor.png)

## Source

`keyboards/splitkb/halcyon/corne/keymaps/mak3r/palette-editor.html` — a single self-contained HTML file, committed like any other source in this repo.

## How to use it

It's built as a Claude Artifact, so the normal way to use it is to ask Claude (in this repo, or any session with access to it) to open or republish it:

- **With Claude's Artifact runtime** (recommended): ask Claude to open `palette-editor.html` as an Artifact. It uses the Artifact `db` capability to save your color choices as you go and read them back later — you can close it and come back without losing work, and ask Claude to pull your saved colors in whenever you're ready to apply them.
- **Without the Artifact runtime**: opening the raw HTML file directly (e.g. in a plain browser) still renders the picker and lets you pick colors, but nothing persists or reaches Claude automatically — use its "View/copy CSV" panel to grab your color choices manually instead.

## The edit → firmware pipeline

Colors picked in the editor aren't live on the keyboard by themselves — they still need to flow through the same generator pipeline as any other `rgb_layers.csv` edit:

```
palette editor (interactive picker)
  -> ask Claude to pull saved colors
    -> rgb_layers.csv regenerated (source of truth, plain CSV)
      -> generate_ledmap.py
        -> ledmap.c (compiled, committed)
          -> rebuild + reflash
```

See `CLAUDE.md`'s "The `mak3r` keymap" section for the full `rgb_layers.csv` format (including its `*` wildcards) and the generator scripts' details.

## Relationship to the desktop HUD

The [`corne-kbd-hud`](https://github.com/mak3r/corne-kbd-hud) desktop app renders each key using the same underlying color data this editor writes (via `rgb_layers.csv`, converted into that app's own `mak3r_layers.json`). Colors picked here won't show up in the HUD automatically — see that repo's `scripts/generate_layout_data.py` and its own [docs/PALETTE_EDITOR.md](https://github.com/mak3r/corne-kbd-hud/blob/main/docs/PALETTE_EDITOR.md) for regenerating its copy of the data after a palette edit here.
