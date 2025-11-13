# RoboCam

Firmware for the AI Thinker ESP32-CAM that steers two servos to keep a detected face centered while exposing a simple HTTP API that always serves the latest rotated snapshot. The device now hosts its own Wi-Fi access point, so no external network is required.

## Features
- Uses `esp32-camera` with the bundled MSR01 + MNP01 neural detectors for face tracking and servo guidance.
- Rotates every captured frame by 90° (matching the mounted camera orientation) **before** face recognition and before serving it to clients.
- Runs an onboard HTTP server with `/` (JSON status) and `/photo` (latest JPEG) endpoints.
- Brings up a stand-alone Wi-Fi AP (`RoboCam-AP` by default) so phones/laptops can connect directly.

## Hardware
- AI Thinker ESP32-CAM module with PSRAM enabled.
- Two hobby servos connected to GPIO 14 (X/pan) and GPIO 15 (Y/tilt). Update `ServoService servoX(14)` / `servoY(15)` in `src/main.cpp` if you use different pins.
- Stable 5V supply capable of powering both the ESP32-CAM and the servos.

## Project Layout
- `src/main.cpp` – application entry point: initializes the servos, camera, AP, and HTTP server.
- `lib/CamController` – camera configuration, face detection pipeline, rotation helper, servo positioning logic, and the JPEG capture wrapper.
- `lib/ServoService` – minimal helper that constrains angles and drives individual servos.
- `include/CONFIG.hpp` – camera pin map plus all runtime configuration macros (detection interval, AP settings, API port, …).
- `platformio.ini` – PlatformIO environment for `esp32cam` (Arduino framework, PSRAM fixes, AI Thinker pinout).
- `Makefile` – quality-of-life wrapper around typical PlatformIO commands (`make build`, `make flash`, `make monitor`, etc.).

## Configuration
Most runtime settings live in `include/CONFIG.hpp`:

| Macro | Purpose | Default |
| --- | --- | --- |
| `FACE_DETECT_INTERVAL` | Minimum delay (ms) between detector runs | `150` |
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
   - `GET /` → `{"status":"ok","ip":"<ap-ip>"}` for a quick health check.
   - `GET /photo` → JPEG stream of the most recent frame (already rotated 90° to match the mounted camera orientation).
3. Disable caching if you fetch images in a browser/script; the firmware already sends `Cache-Control: no-store`.

## Face Tracking Behavior
- Every frame is rotated before entering the MSR01/MNP01 detector chain so that the neural network sees the same orientation that you do on `/photo`.
- When a face is detected, the controller computes the center error and nudges each servo by a clamped step (`±5°` max) scaled to the frame size.
- If no face is present, the servos remain at their last angles and the log prints “No face detected”.

## Troubleshooting
- Reboots or brownouts usually indicate insufficient 5V current while the servos move; try powering the servos separately with a shared ground.
- If the serial monitor shows `Camera capture failed`, confirm the ribbon cable, pin map, and that PSRAM is enabled in `platformio.ini`.
- Should the AP fail to start, double-check that the password is at least 8 characters and that no other device nearby uses the same channel with overlapping SSID.
