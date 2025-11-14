# RoboCam

Firmware for the AI Thinker ESP32-CAM that exposes a low-latency livestream plus a web-based controller to switch between manual pan/tilt or autonomous face tracking. Detections power both the on-frame overlay and the optional auto-centering logic, while the device still hosts its own Wi-Fi access point—no external network required.

## Features
- Uses `esp32-camera` with the bundled MSR01 + MNP01 neural detectors to both annotate frames and (optionally) keep faces centered with gentle servo nudges.
- Streams the native camera orientation directly to both the detector pipeline and the HTTP endpoints (no extra rotation step).
- Configures the OV2640 sensor for ultra-wide lenses: VGA frames, lens correction, AWB/AEC tweaks, and JPEG quality tuned for livestreaming.
- Draws a red bounding box around the latest detected face directly on the JPEG stream, so the `/` livestream doubles as a visual debugger.
- Streams native sensor JPEGs for the MJPEG feed while the browser overlays the face box via a canvas fed by the `/status` endpoint.
- Includes built-in pan/tilt arrow buttons (backed by an HTTP API) so you can manually steer the camera from the browser when needed, plus a mode toggle to switch to autonomous tracking.
- Adjustable livestream quality presets (High/Medium/Low) directly from the browser, balancing latency vs. compression without changing the sensor resolution.
- Dedicated MJPEG stream server on port `81` keeps video smooth even while the REST API serves UI/controls on port `80`.
- Runs an onboard HTTP server with `/` (livestream UI), `/status` (JSON health check), and `/photo` (latest JPEG) endpoints.
- Brings up a stand-alone Wi-Fi AP (`RoboCam-AP` by default) so phones/laptops can connect directly.

## Hardware
- AI Thinker ESP32-CAM module with PSRAM enabled.
- Two hobby servos connected to GPIO 14 (X/pan) and GPIO 15 (Y/tilt). Update `ServoService servoX(14)` / `servoY(15)` in `src/main.cpp` if you use different pins.
- Stable 5V supply capable of powering both the ESP32-CAM and the servos.

## Project Layout
- `src/main.cpp` – application entry point: initializes the servos, camera, AP, and HTTP server.
- `lib/CamController` – camera configuration, face detection pipeline, servo positioning logic, and the JPEG capture wrapper.
- `lib/ServoService` – minimal helper that constrains angles and drives individual servos.
- `lib/WebService` – encapsulates the HTTP server, endpoints, and the gzipped UI asset delivery.
- `include/CONFIG.hpp` – camera pin map plus all runtime configuration macros (detection interval, AP settings, API port, …).
- `platformio.ini` – PlatformIO environment for `esp32cam` (Arduino framework, PSRAM fixes, AI Thinker pinout).
- `Makefile` – quality-of-life wrapper around typical PlatformIO commands (`make build`, `make flash`, `make monitor`, etc.).

## Configuration
Most runtime settings live in `include/CONFIG.hpp`:

| Macro | Purpose | Default |
| --- | --- | --- |
| `FACE_DETECT_INTERVAL` | Delay (ms) between autonomous tracking updates | `150` |
| `AP_SSID` | SSID broadcast by the ESP32-CAM | `RoboCam-AP` |
| `AP_PASSWORD` | WPA2 password (≥8 chars) | `robocam123` |
| `AP_CHANNEL` | Wi-Fi channel for the AP | `1` |
| `AP_MAX_CONNECTIONS` | Max simultaneous STA clients | `1` |
| `API_PORT` | HTTP server port | `80` |

Camera pin assignments (`PWDN_GPIO_NUM`, `Y2_GPIO_NUM`, etc.) already match the AI Thinker module. Tweak them only if you use a custom board.

## Build & Flash
1. Install [PlatformIO](https://platformio.org/) (CLI or VS Code extension).
2. From the project root, build with `pio run -e esp32cam` (or `make build`).
3. Flash: `pio run -e esp32cam -t upload` or `make flash [PORT_NUMBER]`.
4. Open the serial monitor at 115200 baud (`pio device monitor -e esp32cam` or `make monitor`) to read boot/AP information.

## Wi-Fi & HTTP API
1. After boot, the ESP32-CAM enables AP mode. Connect to the SSID printed on the serial output (default `RoboCam-AP`) using the configured password.
2. The AP IP defaults to `192.168.4.1`. Browse to:
   - `GET /` → Browser-based livestream UI (uses `/photo` for snapshots, `/stream` for video).
   - `GET /status` → `{"status":"ok","ip":"<ap-ip>","mode":"manual|auto","qualityValue":12,"frameSizeLabel":"VGA (640x480)"}`.
   - `GET /photo` → Always-fresh JPEG snapshot with the current face bounding box burned into the image.
   - `GET http://<ap-ip>:81/stream` → Continuous MJPEG stream (used by the UI).
   - `GET /mode` → `{"mode":"manual|auto"}` to inspect the tracking state.
   - `GET /quality` → `{"preset":"high|medium|low|custom","quality":<jpeg-value>,"frameSizeLabel":"..."}` to inspect the active preset/resolution.
   - `POST /servo/move` → Form-encoded body with `axis=pan|tilt` and `delta=<int>` (±15° max per request) to nudge the servos (fails with HTTP 409 while auto mode runs).
   - `POST /mode` → Form-encoded `mode=manual|auto` to switch between manual joystick and autonomous tracking.
   - `POST /quality` → Form-encoded `preset=high|medium|low` to jump between resolution profiles, or `value=<10-63>` for a custom JPEG quality (resolution stays on the previous preset).
3. Disable caching if you fetch images in your own browser/script; the firmware already sends `Cache-Control: no-store`.

## Face Detection & Tracking
- Every requested frame is piped through the MSR01/MNP01 detector chain **only when `/photo` (or the `/` livestream) is actively fetching data**, so there is zero CPU cost when nobody is watching.
- The detector output burns a red rectangle into the RGB565 buffer before JPEG compression. When autonomous mode is enabled, the same data is reused every `FACE_DETECT_INTERVAL` to nudge the servos by up to ±3° per axis so the subject stays centered.
- Switch back to manual mode whenever you need deterministic positioning; the firmware stops issuing servo commands immediately.

## Camera Quality Preset
- `CamController::begin` sets the camera to `FRAMESIZE_VGA` (640×480) while keeping the detector-friendly `PIXFORMAT_RGB565`.
- Sensor tuning applies brightness `1`, enables lens correction (`lenc`), AWB/AEC, AGC with `GAINCEILING_2X`, raw gamma, and a JPEG quality target of `12`.
- `CamController::captureJpegWithFaceBox()` reuses the same frame to run face detection on-demand, draws a red rectangle on the RGB565 buffer, and only then compresses it to JPEG for `/photo`.
- Adjust the tuning values inside `lib/CamController/CamController.cpp::applyUltraWideSensorPreset()` if your lighting or lens differs. Lower JPEG quality numbers mean higher visual quality but larger frames; values between `10` and `16` typically balance sharpness and Wi-Fi bandwidth.

## Manual Servo Control
- The `/` UI exposes a four-way “joystick” composed of arrow buttons. Each click sends a ±5° delta to the firmware (clamped to ±15° server-side) so you can align the camera even when no face is present.
- Under the hood those buttons call `POST /servo/move` with a simple form body. The JSON response echoes back the axis identifier (`pan` or `tilt`), the applied delta, and the resulting absolute angle (`0–180°`).
- You can hit the endpoint from your own scripts or mobile apps; the firmware rejects the request (HTTP 409) if autonomous mode is active to avoid fighting the tracker.
- Both servos boot at the 90° midpoint, so you immediately have travel available in every direction.

## Tracking Modes
- The `/` UI toggle (or `POST /mode`) switches between **Manuell** (joystick-only) and **Autonom** (face-tracked nudges). The current mode is also reported by `GET /status` and `GET /mode`.
- Manual mode keeps the camera completely idle until a user presses a button, maximizing FPS for the livestream.
- Autonomous mode reuses the latest detections to steer by up to ±3° and only runs at `FACE_DETECT_INTERVAL` cadence, so it adds minimal CPU overhead while keeping subjects centered.

## Livestream Quality Presets
- Each preset adjusts *both* the sensor resolution and JPEG compression to balance FPS vs. detail:
  - `High` → `FRAMESIZE_XGA` (1024×768) @ JPEG quality `12`.
  - `Medium` → `FRAMESIZE_VGA` (640×480) @ JPEG quality `18`.
  - `Low` → `FRAMESIZE_QVGA` (320×240) @ JPEG quality `24` (best latency).
- Use the dropdown on `/` or `POST /quality` to change the preset; the firmware reconfigures the sensor immediately so `/photo` and `/stream` both reflect the new resolution. Move the slider (10–63) when you just need a different compression level—the UI switches to “Custom” and keeps the current resolution.

## Web UI Asset Pipeline
- The readable source lives at `web/index.html`. Edit this file to change the livestream UI (HTML/CSS/JS).
- Regenerate the compressed asset before building by running `python3 tools/build_web_ui.py` (or `make webui`). The script minifies (when `html-minifier` is available), gzips, and emits `include/web_ui_gz.h`, which the firmware serves with `Content-Encoding: gzip`.
- Detailed cross-platform instructions (plain `html-minifier`/`gzip`/`xxd` commands for Linux/macOS/Windows) are documented in `docs/WebUI.md` if you prefer doing the steps manually.

## Troubleshooting
- Reboots or brownouts usually indicate insufficient 5V current while the servos move; try powering the servos separately with a shared ground.
- If the serial monitor shows `Camera capture failed`, confirm the ribbon cable, pin map, and that PSRAM is enabled in `platformio.ini`.
- Should the AP fail to start, double-check that the password is at least 8 characters and that no other device nearby uses the same channel with overlapping SSID.
