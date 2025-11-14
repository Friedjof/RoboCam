# Web UI Asset Pipeline

The firmware now serves a single gzipped HTML asset from flash. Edit `web/index.html`, regenerate the compressed header, flash — no more editing giant `const char[]` blobs in `main.cpp`.

## Quick Path (Recommended)
1. Update `web/index.html` — this file is the readable copy of the livestream UI (HTML, CSS, JS).
2. Run the helper script (or the new Make target) before building the firmware:
   ```bash
   python3 tools/build_web_ui.py
   # or
   make webui
   ```
   The script will:
   - try to call `html-minifier` when it is installed (falls back to the original HTML when it isn’t);
   - gzip the result at level 9;
   - emit `include/web_ui_gz.h` with a PROGMEM array + length constant (`WEB_UI_HTML_GZ`, `WEB_UI_HTML_GZ_len`).
3. Build/flash as usual (`make build`, `make flash`, …). `handleLivestreamRequest()` now streams `WEB_UI_HTML_GZ` with `Content-Encoding: gzip` automatically, and the `<img>` inside `web/index.html` points to the MJPEG endpoint on port `81` (`http://<device-ip>:81/stream`).

## Manual Workflow (If You Prefer CLI Steps)
The following mirrors the detailed instructions shared earlier and lets you regenerate the header without the Python helper.

### Requirements
| Platform | Tools |
| --- | --- |
| Linux (Ubuntu 24.04 LTS) | `sudo apt install xxd gzip html-minifier` |
| macOS | `brew install html-minifier` (xxd already present) |
| Windows | [Vim for Windows](https://www.vim.org/download.php) for `xxd`, plus [7-Zip](https://www.7-zip.org/) or PowerShell for gzip |

### Steps
1. **Create or edit** `web/index.html` (example markup in the repository).
2. **Minify** (optional but recommended):
   ```bash
   html-minifier --collapse-whitespace --remove-comments \
     --remove-optional-tags --remove-redundant-attributes \
     --minify-css true --minify-js true \
     web/index.html -o web/index.min.html
   ```
3. **Compress** the minified file:
   ```bash
   gzip -9 -k web/index.min.html    # Linux/macOS
   ```
   Windows alternatives:
   - 7-Zip: right-click → “7-Zip → Add to archive…”, choose `gzip`, compression “Ultra”.
   - PowerShell:
     ```powershell
     $input  = [IO.File]::ReadAllBytes('web/index.min.html')
     $output = [IO.File]::Create('web/index.min.html.gz')
     $gzip   = New-Object IO.Compression.GzipStream($output, [IO.Compression.CompressionMode]::Compress)
     $gzip.Write($input, 0, $input.Length)
     $gzip.Close(); $output.Close()
     ```
4. **Convert to a header** (pick one method):
   - With `xxd`:
     ```bash
     xxd -i web/index.min.html.gz > include/web_ui_gz.h
     ```
   - With Python (no xxd required):
     ```python
     import gzip, pathlib
     data = pathlib.Path('web/index.min.html.gz').read_bytes()
     out = pathlib.Path('include/web_ui_gz.h')
     out.write_text('const unsigned char WEB_UI_HTML_GZ[] PROGMEM = {\n  ')
     # ... (see tools/build_web_ui.py for a complete example)
     ```
5. **Serve it**: ensure `handleLivestreamRequest()` (already wired in this repo) calls `send_P(... WEB_UI_HTML_GZ ...)` and adds the `Content-Encoding: gzip` header.

### Notes & Tips
- Always keep `web/index.html` under version control — it’s the source of truth.
- `tools/build_web_ui.py` already performs steps 2–4 (and cleans up temp files). Use it unless you *have* to run the manual sequence.
- If you switch to another HTML file name, pass it via `python3 tools/build_web_ui.py --source path/to/file.html --array MY_ARRAY_NAME`.
