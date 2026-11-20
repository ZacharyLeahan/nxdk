#!/usr/bin/env python3
"""Render the xemu queue-ordering witness used by the upstream PR."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


WIDTH, HEIGHT = 1200, 675
BG = "#0d1117"
PANEL = "#161b22"
BORDER = "#30363d"
TEXT = "#f0f6fc"
MUTED = "#8b949e"
BLUE = "#58a6ff"
GREEN = "#3fb950"
RED = "#f85149"
AMBER = "#d29922"


def font(size: int, bold: bool = False):
    windows = Path("C:/Windows/Fonts")
    names = ["seguisb.ttf", "arialbd.ttf"] if bold else ["segoeui.ttf", "arial.ttf"]
    for name in names:
        candidate = windows / name
        if candidate.exists():
            return ImageFont.truetype(str(candidate), size)
    return ImageFont.load_default(size=size)


F_TITLE = font(34, True)
F_SUBTITLE = font(21)
F_PANEL = font(25, True)
F_BODY = font(21)
F_SMALL = font(17)
F_FRAME = font(28, True)


def centered(draw, box, label, used_font, fill=TEXT):
    x0, y0, x1, y1 = box
    bounds = draw.textbbox((0, 0), label, font=used_font)
    x = x0 + (x1 - x0 - (bounds[2] - bounds[0])) / 2
    y = y0 + (y1 - y0 - (bounds[3] - bounds[1])) / 2 - bounds[1]
    draw.text((x, y), label, font=used_font, fill=fill)


def pill(draw, x, y, label, color, width=148):
    box = (x, y, x + width, y + 58)
    draw.rounded_rectangle(box, 12, fill="#21262d", outline=color, width=3)
    centered(draw, box, label, F_FRAME, color)


def panel(draw, x, title, title_color, current, queued, note, note_color):
    draw.rounded_rectangle((x, 128, x + 550, 574), 18, fill=PANEL, outline=BORDER, width=2)
    draw.text((x + 26, 150), title, font=F_PANEL, fill=title_color)
    draw.text((x + 26, 199), "SCANOUT", font=F_SMALL, fill=MUTED)
    draw.text((x + 274, 199), "READY QUEUE", font=F_SMALL, fill=MUTED)
    pill(draw, x + 26, 232, current[0], current[1], 190)
    if queued:
        for index, item in enumerate(queued):
            pill(draw, x + 274, 232 + index * 72, item[0], item[1], 235)
    else:
        draw.text((x + 274, 248), "empty", font=F_BODY, fill=MUTED)
    draw.line((x + 26, 409, x + 524, 409), fill=BORDER, width=2)
    draw.multiline_text((x + 26, 432), note, font=F_BODY, fill=note_color, spacing=8)


STEPS = [
    {
        "heading": "Frame 127 is complete and queued",
        "stock": (("frame 126", BLUE), [("frame 127", AMBER)], "queued frame=127 surface=1", TEXT),
        "patched": (("frame 126", BLUE), [("frame 127", AMBER)], "queued frame=127 surface=1", TEXT),
    },
    {
        "heading": "The Xbox repeats frame 126 for one refresh",
        "stock": (("frame 126", BLUE), [("frame 127", AMBER)], "repeated refresh\nis normal by itself", MUTED),
        "patched": (("frame 126", BLUE), [("frame 127", AMBER)], "repeated refresh\nis normal by itself", MUTED),
    },
    {
        "heading": "The DPC runs before scanout advances",
        "stock": (("frame 126", BLUE), [], "REMOVED frame 127\nbefore selection", RED),
        "patched": (("frame 126", BLUE), [("frame 127", AMBER)], "DEFER removal:\nscanout does not match", GREEN),
    },
    {
        "heading": "The next VBlank selects a buffer",
        "stock": (("frame 128", BLUE), [], "frame 127 is missing", RED),
        "patched": (("frame 127", GREEN), [("frame 128", AMBER)], "SELECT frame 127,\nthen retire it", GREEN),
    },
    {
        "heading": "Observed ordering",
        "stock": (("frame 128", BLUE), [], "126  →  128\nPREMATURE REMOVAL", RED),
        "patched": (("frame 128", GREEN), [], "126  →  127  →  128\nIN ORDER", GREEN),
    },
]


def render(step_index: int):
    step = STEPS[step_index]
    image = Image.new("RGB", (WIDTH, HEIGHT), BG)
    draw = ImageDraw.Draw(image)
    draw.text((36, 26), "pbkit display-queue race", font=F_TITLE, fill=TEXT)
    draw.text((36, 76), step["heading"], font=F_SUBTITLE, fill=MUTED)
    panel(draw, 35, "STOCK", RED, *step["stock"])
    panel(draw, 615, "PATCHED", GREEN, *step["patched"])
    draw.text((36, 616), "Trace-driven animation from the xemu ordering witness; repeated refreshes are not failures.", font=F_SMALL, fill=MUTED)
    draw.text((1127, 616), f"{step_index + 1}/{len(STEPS)}", font=F_SMALL, fill=MUTED, anchor="ra")
    return image


def main():
    output = Path(__file__).with_name("pbkit-queue-ordering.gif")
    frames = [render(i) for i in range(len(STEPS))]
    frames[0].save(
        output,
        save_all=True,
        append_images=frames[1:],
        duration=[1400, 1600, 1900, 1900, 3000],
        loop=0,
        optimize=True,
        disposal=2,
    )
    print(output)


if __name__ == "__main__":
    main()
