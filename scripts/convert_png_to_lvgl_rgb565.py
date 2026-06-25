#!/usr/bin/env python3
"""Convert a square PNG to an LVGL 8 RGB565 (LV_IMG_CF_TRUE_COLOR) C asset.

Requires Pillow for regeneration only. Generated .c/.h files are committed so
normal firmware builds do not need Pillow.

Pipeline:
  1. Open PNG as RGBA
  2. Resize/crop to target square (uniform, no letterbox)
  3. Alpha-composite onto solid black (never src.convert("RGB") on RGBA directly)
  4. Convert to RGB565 byte pairs matching lv_color_t / LV_COLOR_16_SWAP

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
import sys
from dataclasses import dataclass


@dataclass
class SourceReport:
    path: str
    reported_mode: str
    width: int
    height: int
    alpha_min: int
    alpha_max: int
    transparent: int
    semi_transparent: int
    opaque: int


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb565_to_rgb888(val: int) -> tuple[int, int, int]:
    r = ((val >> 11) & 0x1F) << 3
    g = ((val >> 5) & 0x3F) << 2
    b = (val & 0x1F) << 3
    return r, g, b


def rgb565_to_bytes(val: int, swap: bool) -> tuple[int, int]:
    lo = val & 0xFF
    hi = (val >> 8) & 0xFF
    if swap:
        return hi, lo
    return lo, hi


def bytes_to_rgb565(lo: int, hi: int, swap: bool) -> int:
    if swap:
        return (lo << 8) | hi
    return (hi << 8) | lo


def analyze_rgba(rgba) -> SourceReport:
    w, h = rgba.size
    pixels = list(rgba.getdata())
    alphas = [p[3] for p in pixels]
    transparent = sum(1 for a in alphas if a == 0)
    semi = sum(1 for a in alphas if 0 < a < 255)
    opaque = sum(1 for a in alphas if a == 255)
    return SourceReport(
        path="",
        reported_mode="RGBA",
        width=w,
        height=h,
        alpha_min=min(alphas),
        alpha_max=max(alphas),
        transparent=transparent,
        semi_transparent=semi,
        opaque=opaque,
    )


def resize_or_cover_to_square(rgba, target: int) -> tuple[object, bool, bool]:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required. Install with: pip install Pillow") from exc

    src_w, src_h = rgba.size
    resized = False
    cropped = False
    im = rgba

    if src_w != target or src_h != target:
        resized = True
        if src_w != src_h:
            side = min(src_w, src_h)
            left = (src_w - side) // 2
            top = (src_h - side) // 2
            im = im.crop((left, top, left + side, top + side))
            cropped = True
        im = im.resize((target, target), Image.Resampling.LANCZOS)

    return im, resized, cropped


def flatten_on_black(rgba) -> object:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required. Install with: pip install Pillow") from exc

    black = Image.new("RGBA", rgba.size, (0, 0, 0, 255))
    return Image.alpha_composite(black, rgba).convert("RGB")


def load_flattened_rgb(path: str, target: int) -> tuple[object, SourceReport, bool, bool]:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required. Install with: pip install Pillow") from exc

    opened = Image.open(path)
    reported_mode = opened.mode
    src_w, src_h = opened.size
    rgba = opened.convert("RGBA")
    report = analyze_rgba(rgba)
    report.path = path
    report.reported_mode = reported_mode

    square_rgba, resized, cropped = resize_or_cover_to_square(rgba, target)
    flattened = flatten_on_black(square_rgba)
    return flattened, report, resized, cropped


def rgb565_bytes_from_image(rgb_image, swap_bytes: bool) -> tuple[list[int], list[tuple[int, int, int]]]:
    pixels = list(rgb_image.getdata())
    out: list[int] = []
    for r, g, b in pixels:
        val = rgb888_to_rgb565(r, g, b)
        lo, hi = rgb565_to_bytes(val, swap_bytes)
        out.extend((lo, hi))
    return out, pixels


def save_rgb565_preview(byte_data: list[int], target: int, path: str, swap_bytes: bool) -> None:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required. Install with: pip install Pillow") from exc

    px: list[tuple[int, int, int]] = []
    for i in range(0, len(byte_data), 2):
        val = bytes_to_rgb565(byte_data[i], byte_data[i + 1], swap_bytes)
        px.append(rgb565_to_rgb888(val))
    preview = Image.new("RGB", (target, target))
    preview.putdata(px)
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    preview.save(path)


def emit_c_source(
    byte_data: list[int],
    target: int,
    symbol: str,
    src_path: str,
    report: SourceReport,
    resized: bool,
    cropped: bool,
    swap_bytes: bool,
) -> str:
    data_size = target * target * 2
    lines: list[str] = []
    lines.append("/* AUTO-GENERATED FILE — DO NOT EDIT */")
    lines.append(f"/* Source: {src_path} ({report.width}x{report.height}, mode={report.reported_mode}) */")
    lines.append(f"/* Target: {target}x{target} RGB565 LV_IMG_CF_TRUE_COLOR */")
    lines.append("/* Alpha: flattened onto black before RGB565 encode */")
    if swap_bytes:
        lines.append("/* RGB565 bytes: high, low (LV_COLOR_16_SWAP style) */")
    else:
        lines.append("/* RGB565 bytes: low, high (native lv_color_t LE) */")
    if resized:
        lines.append("/* Resized: yes */")
    if cropped:
        lines.append("/* Center-crop: yes */")
    lines.append("")
    lines.append('#include "circe_homepage_bg.h"')
    lines.append("")
    lines.append(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {symbol}_map[] = {{")

    row: list[str] = []
    for b in byte_data:
        row.append(f"0x{b:02x}")
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


def print_report(
    report: SourceReport,
    target: int,
    byte_data: list[int],
    resized: bool,
    cropped: bool,
    swap_bytes: bool,
    preview_path: str,
    rgb565_preview_path: str,
) -> None:
    expected = target * target * 2
    print("=== CIRCE PNG → RGB565 conversion report ===")
    print(f"Source path: {report.path}")
    print(f"Source reported mode: {report.reported_mode}")
    print(f"Source dimensions: {report.width}x{report.height}")
    print(f"Alpha min/max: {report.alpha_min}/{report.alpha_max}")
    print(f"Transparent pixels: {report.transparent}")
    print(f"Semi-transparent pixels: {report.semi_transparent}")
    print(f"Opaque pixels: {report.opaque}")
    print(f"Generated dimensions: {target}x{target}")
    print("LVGL format: LV_IMG_CF_TRUE_COLOR (RGB565)")
    print(f"Estimated data size: {expected} bytes")
    print(f"Actual data size: {len(byte_data)} bytes")
    print(f"width*height*2 match: {len(byte_data) == expected}")
    print(f"Resized: {resized}")
    print(f"Center-crop: {cropped}")
    print(f"RGB565 byte order: {'swap (hi,lo)' if swap_bytes else 'native (lo,hi)'}")
    print(f"Flattened preview: {preview_path}")
    print(f"RGB565 roundtrip preview: {rgb565_preview_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert PNG to LVGL RGB565 C asset")
    parser.add_argument("png", help="Source PNG path")
    parser.add_argument("--target-size", type=int, default=412, help="Output square size")
    parser.add_argument("--out-c", required=True, help="Output .c path")
    parser.add_argument("--out-h", required=True, help="Output .h path")
    parser.add_argument("--symbol", default="circe_homepage_bg", help="C symbol base name")
    parser.add_argument(
        "--preview",
        default="",
        help="Flattened RGB preview PNG path (default: next to .c as .preview.png)",
    )
    parser.add_argument(
        "--rgb565-preview",
        default="",
        help="RGB565 roundtrip preview PNG path (default: .rgb565_preview.png)",
    )
    parser.add_argument(
        "--swap-bytes",
        action="store_true",
        help="Emit high,low byte pairs (only if LV_COLOR_16_SWAP=1)",
    )
    args = parser.parse_args()

    flattened, report, resized, cropped = load_flattened_rgb(args.png, args.target_size)
    if flattened.size != (args.target_size, args.target_size):
        print("flattened size mismatch", file=sys.stderr)
        return 1

    preview_path = args.preview or os.path.splitext(args.out_c)[0] + ".preview.png"
    rgb565_preview_path = args.rgb565_preview or os.path.splitext(args.out_c)[0] + ".rgb565_preview.png"

    byte_data, _ = rgb565_bytes_from_image(flattened, args.swap_bytes)
    if len(byte_data) != args.target_size * args.target_size * 2:
        print("byte payload size mismatch", file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(args.out_c), exist_ok=True)
    os.makedirs(os.path.dirname(args.out_h), exist_ok=True)
    os.makedirs(os.path.dirname(preview_path) or ".", exist_ok=True)

    flattened.save(preview_path)
    save_rgb565_preview(byte_data, args.target_size, rgb565_preview_path, args.swap_bytes)

    c_text = emit_c_source(
        byte_data,
        args.target_size,
        args.symbol,
        args.png,
        report,
        resized,
        cropped,
        args.swap_bytes,
    )
    h_text = emit_header(args.symbol, args.target_size)

    with open(args.out_c, "w", encoding="utf-8") as f:
        f.write(c_text)
    with open(args.out_h, "w", encoding="utf-8") as f:
        f.write(h_text)

    print_report(
        report,
        args.target_size,
        byte_data,
        resized,
        cropped,
        args.swap_bytes,
        preview_path,
        rgb565_preview_path,
    )
    print(f"Wrote {args.out_c}")
    print(f"Wrote {args.out_h}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
