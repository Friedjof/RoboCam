# ESP32Cam Firmware 📹

ESP32-CAM Firmware für das RoboCam-System mit Live-Video-Streaming und Servo-Steuerung.

## 🎯 Features

- **WebSocket-Server** auf Port 80
- **Live Video Streaming** über binäre WebSocket-Nachrichten
- **Servo-Motor-Steuerung** (Pan/Tilt) mit Smooth-Easing
- **WS2812 RGB-LED-Strip** Unterstützung
- **Multi-Client-Support** mit konfigurierbarem Limit
- **Eingebautes Web-Interface** mit responsivem Design
- **JSON-API** für Positions- und Limitabfragen

## 🏗️ Hardware-Anforderungen

### Komponenten
- **ESP32-CAM Modul** (AI-Thinker oder kompatibel)
- **2x Servo-Motoren** (SG90 oder ähnlich)
- **WS2812 LED-Strip** (optional)
- **Stromversorgung** 5V/2A (empfohlen)

### Pin-Belegung
```
ESP32-CAM Pin → Komponente
GPIO 14       → Servo 1 (X-Achse/Pan)
GPIO 2        → Servo 2 (Y-Achse/Tilt)
GPIO 4        → WS2812 Data Pin
GPIO 33       → Onboard LED (Status)
GND           → Gemeinsame Masse
5V            → Servo/LED Stromversorgung
```

## 🚀 Installation

### 1. Entwicklungsumgebung

**PlatformIO** (empfohlen):
```bash
# PlatformIO Core installieren
pip install platformio

# Projekt kompilieren
pio run

# Firmware flashen
pio run --target upload

# Serial Monitor
pio device monitor
```

**Arduino IDE**:
- ESP32 Board Package installieren
- Erforderliche Libraries installieren (siehe platformio.ini)

### 2. Konfiguration

Erstellen Sie die Konfigurationsdatei:

```bash
cp include/config.h-template include/config.h
```

Bearbeiten Sie `include/config.h`:

```cpp
// WLAN-Konfiguration
const char* const WIFI_SSID = "IhrWLAN";
const char* const WIFI_PASSWORD = "IhrPasswort";

// Servo-Konfiguration
#define GPIO_SERVO_1  14        // X-Achse Pin
#define GPIO_SERVO_2  2         // Y-Achse Pin
#define START_POS_X   90        // Start-Position X (0-180°)
#define START_POS_Y   45        // Start-Position Y (0-180°)
#define MIN_POS_X     0         // Minimum X Position
#define MAX_POS_X     180       // Maximum X Position
#define MIN_POS_Y     0         // Minimum Y Position
#define MAX_POS_Y     180       // Maximum Y Position

// Performance-Einstellungen
#define MAX_CLIENTS   5         // Maximale WebSocket-Clients
#define FRAME_INTERVAL 100      // Frame-Intervall in ms (10 FPS)

// LED-Konfiguration
#define GPIO_LED_STRIP 4        // WS2812 Data Pin
#define LED_COUNT     8         // Anzahl LEDs im Strip
```

### 3. Build & Flash

Mit Make (vereinfacht):
```bash
# Alle Targets anzeigen
make help

# Kompilieren und flashen
make flash

# Nur kompilieren
make build

# Serial Monitor
make monitor

# Clean Build
make clean
```

## 🎮 Verwendung

### 1. Erste Inbetriebnahme

1. **ESP32 mit Strom versorgen**
2. **Serial Monitor öffnen** (115200 baud)
3. **IP-Adresse notieren** (wird nach WiFi-Verbindung ausgegeben)
4. **Browser öffnen**: `http://ESP32_IP_ADRESSE`

### 2. Web-Interface

Das eingebaute Interface bietet:
- **Live-Video-Stream** (800x600px)
- **Richtungstasten** für Servo-Steuerung
- **Zentrier-Button** für Grundposition
- **Präzisions-Slider** für X/Y-Achsen
- **Licht-Steuerung** für LED-Strip
- **Client-Counter** (live updates)

## 🔧 API-Referenz

### WebSocket-Verbindung

```javascript
const ws = new WebSocket('ws://ESP32_IP/ws');
ws.binaryType = 'blob';
```

### Kommandos (String)

| Kommando | Beschreibung | Antwort |
|----------|--------------|---------|
| `"getframe"` | Einzelnen JPEG-Frame anfordern | Binäre JPEG-Daten |
| `"up"` | Y-Position um 10° reduzieren | Position JSON |
| `"down"` | Y-Position um 10° erhöhen | Position JSON |
| `"left"` | X-Position um 10° erhöhen | Position JSON |
| `"right"` | X-Position um 10° reduzieren | Position JSON |
| `"center"` | Auf START_POS_X/Y zurücksetzen | Position JSON |
| `"get_pos"` | Aktuelle Position abrufen | `{"x":90,"y":45}` |
| `"get_limits"` | Bewegungsgrenzen abrufen | `{"min_x":0,"max_x":180,...}` |
| `"client_count"` | Anzahl verbundener Clients | `{"client_count":2}` |
| `"light_on"` | LED-Strip einschalten | - |
| `"light_off"` | LED-Strip ausschalten | - |

### JSON-Kommandos

Direkte Positionierung:
```javascript
ws.send(JSON.stringify({x: 90, y: 45}));
// Antwort: {"x":90,"y":45}
```

### Event Handling

```javascript
ws.onmessage = function(event) {
    if (typeof event.data === 'string') {
        // JSON-Antworten verarbeiten
        const data = JSON.parse(event.data);
        console.log('Position:', data.x, data.y);
    } else {
        // Binäre Bilddaten (JPEG-Frame)
        const url = URL.createObjectURL(event.data);
        document.getElementById('image').src = url;
    }
};
```

## 📁 Code-Struktur

```
ESP32Cam/
├── src/
│   └── main.cpp              # Hauptprogramm mit WebSocket-Server
├── lib/                      # Custom Libraries
│   ├── cam/
│   │   ├── cam.hpp           # Camera-Klasse Header
│   │   └── cam.cpp           # Kamera-Initialisierung & Frame-Capture
│   ├── robo/
│   │   ├── robo.hpp          # Robo-Klasse Header
│   │   └── robo.cpp          # Servo-Steuerung mit Smooth-Easing
│   └── light/
│       ├── light.hpp         # Light-Klasse Header
│       └── light.cpp         # WS2812 LED-Strip Steuerung
├── include/
│   ├── config.h              # Ihre Konfiguration (zu erstellen)
│   └── config.h-template     # Konfigurationsvorlage
├── platformio.ini            # PlatformIO Konfiguration
└── Makefile                  # Build-Automatisierung
```

## 🔬 Klassen-Übersicht

### Camera-Klasse

```cpp
#include "cam.hpp"

Camera camera;
camera.init();                    // Kamera initialisieren
camera.connectToWiFi();          // WiFi-Verbindung herstellen
camera_fb_t* frame = camera.getFrame();  // Frame erfassen
camera.returnFrame(frame);       // Frame-Buffer freigeben
```

### Robo-Klasse (Servo-Steuerung)

```cpp
#include "robo.hpp"

Robo robo;
robo.init();                     // Servos initialisieren
robo.setPosition(90, 45);        // Position setzen (mit Easing)
int x = robo.getPositionX();     // Aktuelle X-Position
int y = robo.getPositionY();     // Aktuelle Y-Position
robo.loop();                     // In main loop() aufrufen
```

### Light-Klasse (LED-Strip)

```cpp
#include "light.hpp"

Light light;
light.init();                    // LED-Strip initialisieren
light.setColor(0xFF0000);        // Farbe setzen (Rot)
light.setBrightness(128);        // Helligkeit (0-255)
light.show();                    // Änderungen anzeigen
light.loop();                    // Effekte-Loop
```

## ⚙️ Performance-Tuning

### Frame-Rate Optimierung

```cpp
// In config.h
#define FRAME_INTERVAL 50        // 20 FPS (50ms Intervall)
#define FRAME_INTERVAL 33        // 30 FPS (33ms Intervall)
#define FRAME_INTERVAL 100       // 10 FPS (100ms Intervall, Standard)
```

### Kamera-Qualität

```cpp
// In cam.cpp - sensor.set_* Funktionen
sensor.set_framesize(FRAMESIZE_SVGA);    // 800x600
sensor.set_quality(12);                  // JPEG-Qualität (0-63, niedriger = besser)
```

### Client-Management

```cpp
// In config.h
#define MAX_CLIENTS 10           // Mehr Clients erlauben
#define WS_MAX_QUEUED_MESSAGES 10 // Größerer WebSocket-Puffer
```

## 🐛 Debugging

### Serial Monitor

```bash
make monitor
# oder
pio device monitor --baud 115200
```

### Debug-Ausgaben

```
Starting up...
Camera init success
WiFi connecting...
Connected to WiFi: YourNetwork
IP address: 192.168.1.100
Setup complete. WebSocket server running.
Client connected: 1
Message from client: up
Client disconnected: 1
```

### Häufige Probleme

1. **Kamera init failed**
   - Stromversorgung prüfen (min. 5V/1A)
   - Kamera-Modul richtig verbunden?

2. **WiFi connection failed**
   - SSID/Passwort in config.h korrekt?
   - 2.4GHz WiFi verfügbar?

3. **Servos bewegen sich nicht**
   - Pin-Konfiguration prüfen
   - Servo-Stromversorgung (5V) vorhanden?

4. **WebSocket connection fails**
   - Firewall blockiert Port 80?
   - Client-Limit erreicht?

## 📊 Monitoring

### Speicher-Verbrauch

```cpp
// Heap-Monitoring in loop()
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

### WiFi-Signal

```cpp
// Signal-Stärke anzeigen
Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
```

## 🔧 Erweiterte Konfiguration

### Kamera-Settings

```cpp
// Erweiterte Kamera-Konfiguration in cam.cpp
sensor.set_brightness(0);     // Helligkeit (-2 bis +2)
sensor.set_contrast(0);       // Kontrast (-2 bis +2)
sensor.set_saturation(0);     // Sättigung (-2 bis +2)
sensor.set_gainceiling(GAINCEILING_128X);  // AGC-Gain
sensor.set_colorbar(true);    // Testbild aktivieren
```

### LED-Effekte

```cpp
// Custom LED-Effekte in light.cpp
light.rainbow();              // Regenbogen-Effekt
light.pulse();                // Pulsierender Effekt
light.strobe();               // Stroboskop-Effekt
```

## 🤝 Entwicklung

### Neue Features hinzufügen

1. **WebSocket-Kommando** in `onWebSocketEvent()` hinzufügen
2. **HTML-Interface** in der eingebetteten Seite erweitern
3. **API-Dokumentation** aktualisieren

### Custom Libraries

Neue Library erstellen:
```bash
mkdir lib/myfeature
touch lib/myfeature/myfeature.hpp
touch lib/myfeature/myfeature.cpp
```

In `platformio.ini` hinzufügen falls erforderlich.

---

**ESP32Cam Firmware** - Teil des RoboCam-Projekts