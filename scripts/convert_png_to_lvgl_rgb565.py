#!/usr/bin/env python3
"""Convert a square PNG to an LVGL 8 RGB565 (LV_IMG_CF_TRUE_COLOR) C asset.

Requires Pillow for regeneration only. Generated .c/.h files are committed so
normal firmware builds do not need Pillow.

Example:
  python3 scripts/convert_png_to_lvgl_rgb565.py \\
    docs/circe_homepage_bg.png \\
    --target-size 412 \\
    --out-c firmware/circe/main/assets/circe_homepage_bg.c \\
    --out-h firmware/circe/main/assets/circe_homepage_bg.h \\
    --symbol circe_homepage_bg
"""

from __future__ import annotations

import argparse
import os
import struct
import sys


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb565_to_bytes(val: int, swap: bool) -> tuple[int, int]:
    lo = val & 0xFF
    hi = (val >> 8) & 0xFF
    if swap:
        return hi, lo
    return lo, hi


def load_resized_rgb(path: str, target: int) -> tuple[list[tuple[int, int, int]], int, int, bool, bool]:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required. Install with: pip install Pillow") from exc

    im = Image.open(path).convert("RGB")
    src_w, src_h = im.size
    resized = False
    cropped = False

    if src_w != target or src_h != target:
        resized = True
        if src_w != src_h:
            side = min(src_w, src_h)
            left = (src_w - side) // 2
            top = (src_h - side) // 2
            im = im.crop((left, top, left + side, top + side))
            cropped = True
        im = im.resize((target, target), Image.Resampling.LANCZOS)

    pixels = list(im.getdata())
    return pixels, src_w, src_h, resized, cropped


def emit_c_source(
    pixels: list[tuple[int, int, int]],
    target: int,
    symbol: str,
    src_path: str,
    src_w: int,
    src_h: int,
    resized: bool,
    cropped: bool,
    swap_bytes: bool,
) -> str:
    data_size = target * target * 2
    lines: list[str] = []
    lines.append("/* AUTO-GENERATED FILE — DO NOT EDIT */")
    lines.append(f"/* Source: {src_path} ({src_w}x{src_h}) */")
    lines.append(f"/* Target: {target}x{target} RGB565 LV_IMG_CF_TRUE_COLOR */")
    if resized:
        lines.append("/* Resized: yes */")
    if cropped:
        lines.append("/* Center-crop: yes */")
    lines.append("")
    lines.append('#include "circe_homepage_bg.h"')
    lines.append("")
    lines.append(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {symbol}_map[] = {{")

    row: list[str] = []
    for r, g, b in pixels:
        val = rgb888_to_rgb565(r, g, b)
        lo, hi = rgb565_to_bytes(val, swap_bytes)
        row.append(f"0x{lo:02x}")
        row.append(f"0x{hi:02x}")
        if len(row) >= 16:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"const lv_img_dsc_t {symbol} = {{")
    lines.append("    .header = {")
    lines.append("        .cf = LV_IMG_CF_TRUE_COLOR,")
    lines.append("        .always_zero = 0,")
    lines.append("        .reserved = 0,")
    lines.append(f"        .w = {target},")
    lines.append(f"        .h = {target},")
    lines.append("    },")
    lines.append(f"    .data_size = {data_size},")
    lines.append(f"    .data = {symbol}_map,")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def emit_header(symbol: str, target: int) -> str:
    return f"""/* AUTO-GENERATED FILE — DO NOT EDIT */
#pragma once

#include "lvgl.h"

extern const lv_img_dsc_t {symbol};

#define CIRCE_HOMEPAGE_BG_W {target}
#define CIRCE_HOMEPAGE_BG_H {target}
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert PNG to LVGL RGB565 C asset")
    parser.add_argument("png", help="Source PNG path")
    parser.add_argument("--target-size", type=int, default=412, help="Output square size")
    parser.add_argument("--out-c", required=True, help="Output .c path")
    parser.add_argument("--out-h", required=True, help="Output .h path")
    parser.add_argument("--symbol", default="circe_homepage_bg", help="C symbol base name")
    parser.add_argument(
        "--swap-bytes",
        action="store_true",
        help="Match LV_COLOR_16_SWAP (SenseCAP CIRCE default: leave unset)",
    )
    args = parser.parse_args()

    pixels, src_w, src_h, resized, cropped = load_resized_rgb(args.png, args.target_size)
    if len(pixels) != args.target_size * args.target_size:
        print("pixel count mismatch", file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(args.out_c), exist_ok=True)
    os.makedirs(os.path.dirname(args.out_h), exist_ok=True)

    c_text = emit_c_source(
        pixels,
        args.target_size,
        args.symbol,
        args.png,
        src_w,
        src_h,
        resized,
        cropped,
        args.swap_bytes,
    )
    h_text = emit_header(args.symbol, args.target_size)

    with open(args.out_c, "w", encoding="utf-8") as f:
        f.write(c_text)
    with open(args.out_h, "w", encoding="utf-8") as f:
        f.write(h_text)

    print(f"Wrote {args.out_c} ({len(pixels) * 2} bytes)")
    print(f"Wrote {args.out_h}")
    print(f"Source {src_w}x{src_h} -> {args.target_size}x{args.target_size} resized={resized} cropped={cropped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
