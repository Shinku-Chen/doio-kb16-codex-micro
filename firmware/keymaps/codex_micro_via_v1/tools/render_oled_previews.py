import argparse
from html import escape
from pathlib import Path


WIDTH = 128
HEIGHT = 32
PREVIEWS = (
    ("via-waiting", "Startup: VIA WAITTING (10 s)"),
    ("via-1", "VIA L0: 1"),
    ("via-2", "VIA L1: 2"),
    ("via-3", "VIA L2: 3"),
    ("via-shortcut", "VIA: actual output / GUI+C"),
    ("codex-logo", "Codex L3: entry screen"),
    ("codex-tasks", "Codex L3: six task-light estimates"),
)


def render(frame, title):
    if len(frame) != WIDTH * HEIGHT // 8:
        raise ValueError(f"Invalid OLED frame size: {len(frame)}")
    pixels = []
    for vertical in range(HEIGHT):
        for horizontal in range(WIDTH):
            if frame[horizontal + vertical // 8 * WIDTH] & (1 << (vertical % 8)):
                pixels.append(f"M{horizontal} {vertical}h1v1h-1z")
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" width="512" height="128" '
        'viewBox="0 0 128 32" role="img" shape-rendering="crispEdges">\n'
        f"  <title>{escape(title)}</title>\n"
        "  <desc>Generated from the firmware OLED test framebuffer; not a hardware photo.</desc>\n"
        '  <rect width="128" height="32" fill="#000"/>\n'
        f'  <path fill="#fff" d="{"".join(pixels)}"/>\n'
        "</svg>\n"
    )


def main():
    parser = argparse.ArgumentParser(description="Render tested 128x32 OLED frames as README SVG previews")
    parser.add_argument("frames", type=Path)
    parser.add_argument("output", type=Path)
    arguments = parser.parse_args()
    arguments.output.mkdir(parents=True, exist_ok=True)
    for name, title in PREVIEWS:
        frame = (arguments.frames / f"{name}.oled").read_bytes()
        destination = arguments.output / f"oled-{name}.svg"
        destination.write_text(render(frame, title), encoding="utf-8")
        print(destination)


if __name__ == "__main__":
    main()
