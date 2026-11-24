#!/usr/bin/env python3
"""Advertise the native PMR1 ring in the built PicoMEM2 option ROM."""

import sys
from pathlib import Path

DESCRIPTOR = b"PMR1" + bytes((1, 16, 0x20, 0x0e, 64, 0, 1, 0, 0, 0, 0, 0))
OFFSET = 0x3fc0


def with_descriptor(image):
    if (len(image) != 0x4000 or image[:3] != b"\x55\xaa\x20"
            or image[6:12] != b"PMBIOS" or sum(image) & 255):
        raise ValueError("expected a valid 16 KiB PMBIOS image")
    if image[OFFSET:-1] != b"\xff" * (len(image) - OFFSET - 1):
        raise ValueError("PMR1 descriptor location is no longer unused ROM padding")
    out = bytearray(image)
    out[OFFSET:OFFSET + len(DESCRIPTOR)] = DESCRIPTOR
    out[-1] = (out[-1] - sum(out)) & 255
    return bytes(out)


if __name__ == "__main__":
    src, dst = map(Path, sys.argv[1:])
    dst.write_bytes(with_descriptor(src.read_bytes()))
