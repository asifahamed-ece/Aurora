#!/usr/bin/env python3
"""
Build a single-file, self-contained copy of the Aurora dashboard.

data/index.html already carries its CSS, JS, and messages inline, so this
script mostly copies it through. The only external reference is the Dreamland
sleeping photo (data/sleeping_chandni.jpg, deliberately gitignored):

  * With --embed-photo it is inlined as a base64 data URI.
  * Without it (and always in CI, where the photo is not in git) the image is
    replaced with a themed placeholder so the page stays self-contained.

The dashboard talks to the device over /api/state and /ws; on a static host
those requests silently fail and the page still renders messages, letters,
and the music box. Output: dist/aurora-dashboard.html
"""

import argparse
import base64
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "data" / "index.html"
PHOTO = ROOT / "data" / "sleeping_chandni.jpg"
OUT = ROOT / "dist" / "aurora-dashboard.html"

# Soft placeholder matching the Strawberry Milkshake theme.
PLACEHOLDER = (
    "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg'"
    " width='600' height='600'><rect width='100%' height='100%' fill='%23ffe4eb'/>"
    "<text x='50%' y='48%' font-family='sans-serif' font-size='30' fill='%23b0768d'"
    " text-anchor='middle'>Dreamland</text>"
    "<text x='50%' y='56%' font-family='sans-serif' font-size='22' fill='%23b0768d'"
    " text-anchor='middle'>Sleeping photo stays on the device</text></svg>"
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--embed-photo",
        action="store_true",
        help="inline data/sleeping_chandni.jpg into the build (local only)",
    )
    args = parser.parse_args()

    if not SRC.exists():
        sys.exit(f"missing {SRC}")

    html = SRC.read_text(encoding="utf-8")

    if args.embed_photo and PHOTO.exists():
        b64 = base64.b64encode(PHOTO.read_bytes()).decode()
        data_uri = f"data:image/jpeg;base64,{b64}"
        print(f"[build] embedding sleeping photo ({PHOTO.stat().st_size // 1024} KB)")
    else:
        data_uri = PLACEHOLDER

    old_img = re.compile(r'<img\b[^>]*src="/?sleeping_chandni\.jpg"[^>]*>')
    if not old_img.search(html):
        print("[build] warning: sleeping_chandni.jpg reference not found")

    html = old_img.sub(
        (
            '<img class="dreamland-img" src="%s"'
            ' alt="Dreamland" style="width:100%%;height:auto;display:block">'
        )
        % data_uri,
        html,
    )

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8")
    print(f"[build] wrote {OUT.relative_to(ROOT)} ({OUT.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()