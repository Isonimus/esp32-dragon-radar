#!/usr/bin/env python3
"""Render the "DRAGON RADAR" title layouts into src/render/dragon_radar_logo.h (ADR-0006).

The firmware cannot rasterise TrueType, so the logo is drawn here once, styled
after the Dragon Ball Z title card, and stored as an ARGB8888 image that
LovyanGFX's pushAlphaImage blends onto whatever background a screen draws.

Usage: python3 scripts/render-logo.py [--preview out.png]
Requires Pillow. Run from the repo root.
"""
import argparse
import math

from PIL import Image, ImageChops, ImageDraw, ImageFont

FONT_PATH = "data/fonts/LuckiestGuy-Regular.ttf"
HEADER_PATH = "src/render/dragon_radar_logo.h"

# Glyphs are drawn at this size, then scaled to fit each layout's box.
EM_PX = 34
# Drawn this many times larger, then downsampled, for smooth edges.
SUPERSAMPLE = 8
OUTLINE_PX = 2.2
WORD_GAP_EM = 0.18

YELLOW = (250, 214, 40)
RED = (226, 32, 24)
OUTLINE = (12, 8, 8)
BALL_ORANGE = (247, 146, 20)
BALL_HIGHLIGHT = (255, 214, 140)
STAR_RED = (222, 30, 20)

# Per-letter (rotation in degrees, vertical offset in ems): the title card's
# lettering is hand-drawn and bounces, which uniform type does not.
# None marks the letter replaced by a dragon ball.
DRAGON = ("DRAGON", YELLOW, [(-4, 0.00), (3, -0.04), (-3, 0.03), (4, -0.03), None, (-3, 0.02)])
RADAR = ("RADAR", RED, [(3, 0.03), (-4, -0.03), (3, 0.02), (-3, -0.04), (4, 0.02)])
# Baseline-to-baseline distance of stacked lines, in capital heights.
LINE_PITCH_CAPS = 1.06

# name -> (max width px, max height px, [(word, line, indent in ems)]).
# Every supported screen is 240 px wide. Most screens leave a ~36 px band above
# their content; the main menu's items start lower, so it gets the title card's
# two stacked lines, the second set in like "BALL Z".
LAYOUTS = {
    "INLINE": (232, 34, [(DRAGON, 0, 0.0), (RADAR, 0, None)]),
    "STACKED": (232, 64, [(DRAGON, 0, 0.0), (RADAR, 1, 0.9)]),
}
# Relative to the capital height; the title card's ball matches its letters.
BALL_DIAMETER_CAPS = 1.0


def glyph_mask(font, char, angle):
    """The rotated glyph, and its centre relative to the text origin."""
    left, top, right, bottom = font.getbbox(char)
    mask = Image.new("L", (right - left, bottom - top), 0)
    ImageDraw.Draw(mask).text((-left, -top), char, font=font, fill=255)
    centre = ((left + right) / 2, (top + bottom) / 2)
    return mask.rotate(angle, resample=Image.BICUBIC, expand=True), centre


def star_points(cx, cy, outer, inner):
    points = []
    for i in range(10):
        radius = outer if i % 2 == 0 else inner
        angle = -math.pi / 2 + i * math.pi / 5
        points.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
    return points


def draw_ball(fill_layer, shape_mask, cx, cy, diameter):
    r = diameter / 2
    box = (cx - r, cy - r, cx + r, cy + r)
    ImageDraw.Draw(shape_mask).ellipse(box, fill=255)
    draw = ImageDraw.Draw(fill_layer)
    draw.ellipse(box, fill=BALL_ORANGE + (255,))
    highlight = r * 0.28
    hx, hy = cx - r * 0.42, cy - r * 0.42
    draw.ellipse((hx - highlight, hy - highlight, hx + highlight, hy + highlight), fill=BALL_HIGHLIGHT + (255,))
    draw.polygon(star_points(cx, cy, r * 0.5, r * 0.2), fill=STAR_RED + (255,))


def render(max_width, max_height, placements):
    em = EM_PX * SUPERSAMPLE
    font = ImageFont.truetype(FONT_PATH, em)
    _, cap_top, _, cap_bottom = font.getbbox("H")
    cap_height = cap_bottom - cap_top
    margin = em // 2
    canvas_w = em * 12
    canvas_h = em * 4

    fill_layer = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    shape_mask = Image.new("L", (canvas_w, canvas_h), 0)
    x = margin
    for (word, colour, letters), line, indent in placements:
        # An indent of None continues the previous word on the same line.
        if indent is None:
            x += WORD_GAP_EM * em
        else:
            x = margin + indent * em
        origin_y = margin + line * LINE_PITCH_CAPS * cap_height
        for char, style in zip(word, letters):
            advance = font.getlength(char)
            if style is None:
                cap_centre_y = origin_y + (cap_top + cap_bottom) / 2
                draw_ball(fill_layer, shape_mask, x + advance / 2, cap_centre_y, cap_height * BALL_DIAMETER_CAPS)
            else:
                angle, rise = style
                mask, (centre_x, centre_y) = glyph_mask(font, char, angle)
                gx = round(x + centre_x - mask.width / 2)
                gy = round(origin_y + centre_y - rise * em - mask.height / 2)
                fill_layer.paste(Image.new("RGBA", mask.size, colour + (255,)), (gx, gy), mask)
                shape_mask.paste(ImageChops.lighter(shape_mask.crop((gx, gy, gx + mask.width, gy + mask.height)), mask), (gx, gy))
            x += advance

    # A round outline: the union of the shape shifted around a circle.
    radius = OUTLINE_PX * SUPERSAMPLE
    outline_mask = shape_mask.copy()
    for step in range(24):
        angle = 2 * math.pi * step / 24
        shifted = ImageChops.offset(shape_mask, round(radius * math.cos(angle)), round(radius * math.sin(angle)))
        outline_mask = ImageChops.lighter(outline_mask, shifted)

    logo = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    logo.paste(Image.new("RGBA", logo.size, OUTLINE + (255,)), (0, 0), outline_mask)
    logo.alpha_composite(fill_layer)
    logo = logo.crop(outline_mask.getbbox())

    scale = min(max_height / logo.height, max_width / logo.width)
    size = (max(1, round(logo.width * scale)), max(1, round(logo.height * scale)))
    # Resize premultiplied, or transparent black bleeds into the edges.
    return logo.convert("RGBa").resize(size, Image.LANCZOS).convert("RGBA")


def logo_array(name, logo):
    pixels = [(a << 24) | (r << 16) | (g << 8) | b for r, g, b, a in logo.getdata()]
    rows = []
    for y in range(logo.height):
        row = pixels[y * logo.width:(y + 1) * logo.width]
        rows.append("    " + ", ".join(f"0x{p:08X}" for p in row) + ",")
    return (
        f"constexpr int DRAGON_RADAR_LOGO_{name}_WIDTH = {logo.width};\n"
        f"constexpr int DRAGON_RADAR_LOGO_{name}_HEIGHT = {logo.height};\n"
        f"static const uint32_t DRAGON_RADAR_LOGO_{name}[] = {{\n" + "\n".join(rows) + "\n};\n\n"
    )


def write_header(logos):
    with open(HEADER_PATH, "w", encoding="ascii") as out:
        out.write(
            "// Generated by scripts/render-logo.py (ADR-0006). Do not edit; re-run the script.\n"
            f"// Font: {FONT_PATH} (Apache 2.0, see data/fonts/LuckiestGuy-LICENSE.txt).\n"
            "#ifndef DRAGON_RADAR_LOGO_H\n#define DRAGON_RADAR_LOGO_H\n\n#include <stdint.h>\n\n"
            "// Row-major 0xAARRGGBB, the layout of lgfx::argb8888_t, for pushAlphaImage.\n\n"
            + "".join(logo_array(name, logo) for name, logo in logos.items())
            + "#endif // DRAGON_RADAR_LOGO_H\n"
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--preview", help="also write an enlarged PNG on the menu background")
    args = parser.parse_args()
    logos = {name: render(*layout) for name, layout in LAYOUTS.items()}
    write_header(logos)
    for name, logo in logos.items():
        print(f"wrote {name} to {HEADER_PATH}: {logo.width}x{logo.height}")
    if args.preview:
        menu_background = (41, 174, 156)
        gap = 8
        sheet = Image.new("RGBA", (240, sum(l.height + gap for l in logos.values())), menu_background + (255,))
        y = gap // 2
        for logo in logos.values():
            sheet.alpha_composite(logo, ((240 - logo.width) // 2, y))
            y += logo.height + gap
        sheet.resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(args.preview)

if __name__ == "__main__":
    main()
