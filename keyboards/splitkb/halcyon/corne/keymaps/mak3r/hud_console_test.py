#!/usr/bin/env python3
"""Standalone diagnostic for hud_console.c's layer-broadcast HUD hook.
Not the real desktop app (that's its own future project) -- this just
listens on the CONSOLE_ENABLE interface and prints what it receives.
Confirmed working on hardware, including with vial.rocks open at the
same time.

Requires: pip3 install --break-system-packages hid
(the `hid` package needs the hidapi native library available on your
system -- on macOS, `brew install hidapi` first if the import fails)
"""
import sys

try:
    import hid
except ImportError:
    print("Missing dependency. Run: pip3 install --break-system-packages hid", file=sys.stderr)
    print("(and `brew install hidapi` on macOS if the import still fails)", file=sys.stderr)
    sys.exit(1)

# QMK's CONSOLE_ENABLE interface always uses this usage page/usage
# (the "PJRC Teensy compatible" convention -- see usb_descriptor.c),
# regardless of the keyboard's VID/PID, so we don't need to hardcode those.
CONSOLE_USAGE_PAGE = 0xFF31
CONSOLE_USAGE = 0x74


def find_console_device():
    matches = [
        d
        for d in hid.enumerate()
        if d.get("usage_page") == CONSOLE_USAGE_PAGE and d.get("usage") == CONSOLE_USAGE
    ]
    return matches


def main():
    matches = find_console_device()
    if not matches:
        print("No QMK console HID interface found.")
        print("Is a mak3r build (CONSOLE_ENABLE) flashed, and the keyboard plugged in?")
        sys.exit(1)

    if len(matches) > 1:
        print(f"Found {len(matches)} console interfaces, using the first:")
        for m in matches:
            print(f"  {m['product_string']!r} path={m['path']!r}")

    info = matches[0]
    print(f"Opening: {info['product_string']!r} (vid={info['vendor_id']:#06x} pid={info['product_id']:#06x})")

    device = hid.Device(path=info["path"])

    print("Listening for layer changes. Switch layers on the keyboard now (Ctrl-C to quit).\n")

    buf = b""
    try:
        while True:
            data = device.read(64, timeout=500)
            if not data:
                continue
            # QMK console reports are padded with trailing zero bytes; strip them.
            chunk = bytes(b for b in data if b != 0)
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                text = line.decode("utf-8", errors="replace").strip()
                if text:
                    print(f"> {text}")
    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        device.close()


if __name__ == "__main__":
    main()
