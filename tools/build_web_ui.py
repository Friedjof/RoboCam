#!/usr/bin/env python3
"""Minify, gzip, and convert the web UI into a PROGMEM header."""
from __future__ import annotations

import argparse
import gzip
import shutil
import subprocess
import sys
from pathlib import Path

def run_html_minifier(source: Path, output: Path) -> bool:
    """Attempt to run the html-minifier CLI. Returns True on success."""
    html_minifier = shutil.which("html-minifier")
    if not html_minifier:
        return False

    cmd = [
        html_minifier,
        "--collapse-whitespace",
        "--remove-comments",
        "--remove-optional-tags",
        "--remove-redundant-attributes",
        "--minify-css",
        "true",
        "--minify-js",
        "true",
        str(source),
        "-o",
        str(output),
    ]

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as exc:
        print(f"[build_web_ui] html-minifier failed: {exc}", file=sys.stderr)
        return False
    return True

def write_header(data: bytes, array_name: str, output: Path) -> None:
    with output.open("w", encoding="utf-8") as fh:
        fh.write("#pragma once\n")
        fh.write("#include <Arduino.h>\n\n")
        fh.write(f"const unsigned char {array_name}[] PROGMEM = {{\n  ")
        for idx, byte in enumerate(data):
            fh.write(f"0x{byte:02x}")
            if idx + 1 != len(data):
                fh.write(", ")
                if (idx + 1) % 16 == 0:
                    fh.write("\n  ")
        fh.write("\n};\n")
        fh.write(f"const unsigned int {array_name}_len = {len(data)};\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default="web/index.html", help="Input HTML file")
    parser.add_argument("--output", default="include/web_ui_gz.h", help="Header path")
    parser.add_argument("--array", default="WEB_UI_HTML_GZ", help="C array symbol name")
    args = parser.parse_args()

    source = Path(args.source)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    if not source.exists():
        print(f"[build_web_ui] Source HTML not found: {source}", file=sys.stderr)
        sys.exit(1)

    minified = source.read_text(encoding="utf-8")

    tmp_minified = source.parent / "index.min.html.tmp"
    if run_html_minifier(source, tmp_minified):
        minified = tmp_minified.read_text(encoding="utf-8")
        try:
            tmp_minified.unlink()
        except FileNotFoundError:
            pass
    else:
        print("[build_web_ui] html-minifier not found; using original HTML", file=sys.stderr)

    compressed = gzip.compress(minified.encode("utf-8"), compresslevel=9)
    write_header(compressed, args.array, output)
    print(f"[build_web_ui] Wrote {output} ({len(compressed)} bytes compressed)")

if __name__ == "__main__":
    main()
