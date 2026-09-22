VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes

ENCODER_MAP_ENABLE = yes

# Layer 3 (Function/Media) uses QK_CAPS_WORD_TOGGLE.
CAPS_WORD_ENABLE = yes

# This adds module functionality to your keyboard (files found in users/halcyon_modules)
USER_NAME := halcyon_modules

SRC += halcyon_overrides.c
SRC += ledmap.c

# Broadcasts the active layer number over a separate USB HID interface
# (see hud_console.c) for a future desktop HUD app to read.
CONSOLE_ENABLE = yes
SRC += hud_console.c
