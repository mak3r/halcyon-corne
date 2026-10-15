# Trackpad Gestures (`mak3r` keymap)

How the Cirque trackpad (right half) behaves with the `mak3r` keymap. All of this runs in firmware, with no host software.

## Mouse layer (layer 4)

Using the trackpad turns on layer 4 automatically (QMK's [auto mouse layer](https://docs.qmk.fm/features/pointing_device#pointing-device-auto-mouse)), so the click keys are ready while your hand stays on the keyboard. The same layer has mouse keys for when you'd rather not reach for the pad (the Typeractive has no trackpad, so it uses them for everything).

| Key | Action |
|---|---|
| A / ; | Left click / right click |
| S and L | Middle click |
| D / F / J / K | Pointer down / right / left / up |
| E / R / U / I | Scroll right / down / up / left |
| Right outer column, top / middle / bottom | Lock pointer speed slow / medium / fast (see below) |
| T | Toggle drag-to-scroll mode (see below) |
| Left middle thumb | Back to layer 0 (`TO(0)`) |
| Right middle thumb | `MO(2)`, the same as on layer 0, for switching workspaces |

**Pointer speed keys**: tap a key in the right outer column (top, middle or bottom) to lock the mouse keys at a steady slow (about 62 px/s, for precise positioning), medium (625 px/s) or fast (2500 px/s) speed. Tap the same key again to go back to normal, where the pointer starts at 250 px/s and speeds up to 1250 px/s over about 0.8 s. Tapping a different speed key switches straight to it. The lock stays on while you move, click and scroll, and it changes scrolling too (1 / 4 / 8 clicks per step). These are QMK's `KC_ACL0`/`1`/`2`, made to latch in `mouse_layer.c` instead of acting only while held.

Layer 4 turns off by itself after ~650ms with no trackpad or click activity, or as soon as you press any non-mouse key. Every other key on layer 4 is transparent.

You can also turn layer 4 on from layer 3:
- `OSL(4)` (D position) holds it for one keypress.
- `TG(4)` (`;` position) latches it on with no timeout, until `TO(0)` or `TG(4)` again.

The desktop HUD deliberately stays hidden on layer 4. Otherwise it would flash up every time you touch the trackpad. Pin the HUD to see this layer.

## Tilt correction

The Cirque module sits in the right half's VIK slot with a slight rotational tilt, so dragging straight "north" (away from you) reads as moving up and slightly to one side rather than straight up. This is corrected in firmware (`apply_cirque_tilt_correction()` in `mouse_layer.c`, applied to every trackpad report before anything else uses it — cursor movement and drag-to-scroll's axis lock both benefit). See **Tuning** below if it needs adjusting for your unit.

## Gestures

**Tap to click**: a quick tap on the pad is a left click.

**Cursor glide**: flick and lift, and the cursor keeps coasting briefly.

**Circular (edge) scroll**: this gesture is strict, so it's worth knowing exactly what triggers it:
1. **Start on the edge.** Your finger must first touch down in the outer ring (roughly the outer third of the radius). Starting near the center and sliding out to the edge is always just cursor movement.
2. **Move along the edge, not toward the center.** After a small movement, the direction decides the gesture. Along the edge (more than 50° off the line toward the center) is a scroll; anything closer to that line becomes normal cursor movement for the rest of that touch.
3. **Where you start picks the axis:**
   - Start on the **right** edge: scroll **up/down**.
   - Start on the **left** edge: scroll **left/right**.
4. **Keep circling** to keep scrolling, about 18 wheel steps per full circle.

**Drag-to-scroll mode**: an alternative to the circular gesture, for when it's hard to land precisely in the edge zone. Tap **T** (layer 4) to toggle it on — the TFT's "Scroll" indicator lights up, same as it does for real Scroll Lock. While it's on, dragging anywhere on the pad scrolls instead of moving the cursor:
- The **first direction you move** after starting a drag picks the axis (horizontal or vertical) for that drag — mirroring how the circular gesture's starting edge picks its axis.
- **Lift your finger and drag again** to pick a different axis; you don't need to toggle the mode off and on to switch between horizontal and vertical.
- Tap **T** again to turn drag-to-scroll off and go back to normal cursor movement (and the circular gesture still works independently, any time).
- Toggling (either direction) disables cursor glide (see above) for as long as drag-to-scroll is active, and discards any leftover axis-lock state from the previous drag. Reported on hardware: without this, toggling right after lifting your trackpad finger could let glide's leftover "coasting" motion get read as a burst of scroll clicks, which some apps (Finder, not Chrome) can turn into a jump to the very top or bottom of the scroll area.

## Tuning

Everything above uses the QMK driver defaults. To change them, edit `keyboards/splitkb/halcyon/corne/keymaps/mak3r/`, then rebuild and reflash:

- **Mouse key speeds**: the `MOUSEKEY_*` defines in `config.h` (the comment there gives the resulting speeds). tya-corne's `MOVE_SPEED`/`SPEED_LAYERS` (in its `scripts/generate_keymap.py`) match them, so change both together.
- **How long layer 4 stays on**: `#define AUTO_MOUSE_TIME <ms>` in `config.h` (default 650).
- **Edge scroll feel**: call `cirque_pinnacle_configure_circular_scroll(outer_ring_pct, trigger_px, trigger_ang, wheel_clicks, left_handed)` from `pointing_device_init_user()` in `mouse_layer.c`. The defaults are `33, 16, 9102 /* 50° */, 18, false`.
  - A larger `outer_ring_pct` makes the edge zone wider.
  - A smaller `trigger_ang` is more forgiving of movement toward the center.
  - A larger `wheel_clicks` scrolls faster.
- **Drag-to-scroll speed**: `#define SCROLL_DRAG_THRESHOLD <n>` in `mouse_layer.c` — raw trackpad units per emitted scroll click. Smaller scrolls faster.
- **Drag-to-scroll axis-switch timing**: `#define SCROLL_DRAG_AXIS_RESET_MS <ms>` in `mouse_layer.c` — how long your finger needs to be off the pad before the next drag can pick a different axis (default 150). Too short and a brief pause mid-drag could "forget" the axis; too long and lifting and redragging quickly won't re-pick it.
- **Drag-to-scroll direction**: flip the sign on `report->v` in `apply_scroll_drag()` (`mouse_layer.c`) if vertical scrolling feels backwards.
- **Tilt correction angle**: `CIRQUE_TILT_COS`/`CIRQUE_TILT_SIN` in `mouse_layer.c`, precomputed for a fixed angle (currently 8°) rather than computed at runtime. To change the angle, recompute both constants for the new value, e.g. `python3 -c "import math; d=10; print(math.cos(math.radians(d)), math.sin(math.radians(d)))"`. If straight-up drag still drifts to one side, or now drifts the other way, try a negative angle instead (same magnitude, flip the sign on both constants) — the correct sign isn't obvious from code alone and needs on-hardware testing.

See `CLAUDE.md`'s "Layer 4 = mouse layer" notes for implementation details and gotchas.
