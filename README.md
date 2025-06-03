# RoboCam 📹🤖

Ein ESP32-basiertes ferngesteuertes Kamerasystem mit Live-Video-Streaming und Servo-Motor-Steuerung.

## 🎯 Features

- **Live Video Streaming** über WebSocket
- **Servo-Motor-Steuerung** für Pan/Tilt-Bewegungen
- **Web-Interface** mit responsivem Design
- **Multi-Client-Support** mit Client-Zähler
- **LED-Beleuchtung** (WS2812 RGB-Strip)
- **Flask-Proxy-Server** für erweiterte Funktionen
- **Benutzer-Authentifizierung** im Proxy-Modus

## 📚 Workshop & Entstehungsgeschichte

Dieses Projekt entstand aus dem **SUSCam Workshop vom 01.06.2024**. Der ursprüngliche Workshop-Code und die Grundlagen findest du hier:

🔗 **[SUSCam Workshop Repository](https://github.com/Friedjof/SUSCam)**

Das RoboCam-Projekt erweitert die Workshop-Grundlagen um erweiterte Features wie Proxy-Server, Authentifizierung und verbesserte Hardware-Steuerung.

## 🏗️ Hardware-Anforderungen

### ESP32Cam
- ESP32-CAM Modul
- 2x Servo-Motoren (SG90 oder ähnlich)
- WS2812 LED-Strip (optional)
- Stromversorgung (5V/2A empfohlen)

### Verkabelung
```
ESP32-CAM Pin → Komponente
GPIO 14       → Servo 1 (X-Achse)
GPIO 2        → Servo 2 (Y-Achse)
GPIO 4        → WS2812 Data Pin
GND           → Servo/LED GND
5V            → Servo/LED VCC
```

## 🚀 Installation

### 1. ESP32Cam Firmware

```bash
# Repository klonen
git clone https://github.com/yourusername/RoboCam.git
cd RoboCam/ESP32Cam

# Konfiguration erstellen
cp include/config.h-template include/config.h
```

Bearbeiten Sie `include/config.h` mit Ihren WiFi-Credentials:

```cpp
const char* const WIFI_SSID = "IhrWLAN";
const char* const WIFI_PASSWORD = "IhrPasswort";
```

Build und Upload mit PlatformIO:

```bash
# Mit Make
make flash

# Oder direkt mit PlatformIO
pio run --target upload
```

### 2. Flask Proxy-Server (Optional)

```bash
cd ../CamProxy

# Python Virtual Environment
python -m venv venv
source venv/bin/activate  # Linux/Mac
# oder: venv\Scripts\activate  # Windows

# Dependencies installieren
pip install -r requirements.txt

# Umgebungsvariablen konfigurieren
cp .env.example .env
# Bearbeiten Sie .env mit Ihren Einstellungen

# Admin-Benutzer erstellen
python scripts/create_admin.py

# Server starten
python app.py
```

## 🎮 Verwendung

### Direkter Zugriff auf ESP32

1. ESP32 mit Strom versorgen
2. IP-Adresse im Serial Monitor notieren
3. Browser öffnen: `http://ESP32_IP_ADRESSE`

### Mit Flask-Proxy

1. Proxy-Server starten
2. Browser öffnen: `http://localhost:5000`
3. Mit Admin-Credentials einloggen

## 🕹️ Steuerung

### Web-Interface
- **Pfeiltasten**: Kamera bewegen
- **Kreis-Button**: Zentrieren
- **Slider**: Präzise Positionierung
- **Licht-Button**: LED-Beleuchtung ein/aus

### WebSocket-Kommandos

| Kommando | Beschreibung | Beispiel |
|----------|--------------|----------|
| `"getframe"` | Einzelnen Frame anfordern | - |
| `"up"/"down"/"left"/"right"` | Servo-Bewegung | `ws.send("up")` |
| `"center"` | Auf Startposition zurück | `ws.send("center")` |
| `{"x":90,"y":45}` | Direkte Positionierung | `ws.send('{"x":90,"y":45}')` |
| `"get_pos"` | Aktuelle Position abrufen | Antwort: `{"x":90,"y":45}` |
| `"get_limits"` | Bewegungsgrenzen abrufen | Antwort: `{"min_x":0,"max_x":180,...}` |
| `"light_on"/"light_off"` | LED-Steuerung | `ws.send("light_on")` |

## 📁 Projektstruktur

```
RoboCam/
├── ESP32Cam/                 # ESP32 Firmware
│   ├── src/
│   │   └── main.cpp          # Hauptprogramm
│   ├── lib/                  # Custom Libraries
│   │   ├── cam/              # Kamera-Klasse
│   │   ├── robo/             # Servo-Steuerung
│   │   └── light/            # LED-Steuerung
│   ├── include/
│   │   ├── config.h          # Konfiguration (zu erstellen)
│   │   └── config.h-template # Konfigurationsvorlage
│   ├── platformio.ini        # PlatformIO Konfiguration
│   └── Makefile              # Build-Automatisierung
├── CamProxy/                 # Flask Web-Server
│   ├── app.py                # Hauptanwendung
│   ├── models.py             # Datenbankmodelle
│   ├── templates/            # HTML-Templates
│   ├── static/               # CSS/JS/Images
│   └── scripts/              # Hilfsskripte
└── README.md                 # Diese Datei
```

## ⚙️ Konfiguration

### ESP32 Einstellungen

In `ESP32Cam/include/config.h`:

```cpp
// WLAN-Konfiguration
const char* const WIFI_SSID = "IhrWLAN";
const char* const WIFI_PASSWORD = "IhrPasswort";

// Servo-Konfiguration
#define GPIO_SERVO_1  14        // X-Achse Pin
#define GPIO_SERVO_2  2         // Y-Achse Pin
#define START_POS_X   90        // Start-Position X
#define START_POS_Y   45        // Start-Position Y
#define MIN_POS_X     0         // Minimum X Position
#define MAX_POS_X     180       // Maximum X Position
#define MIN_POS_Y     0         // Minimum Y Position
#define MAX_POS_Y     180       // Maximum Y Position

// Performance-Einstellungen
#define MAX_CLIENTS   5         // Maximale WebSocket-Clients
#define FRAME_INTERVAL 100      // Frame-Intervall in ms
```

### Flask-Proxy Einstellungen

In `CamProxy/.env`:

```bash
SECRET_KEY=ihr_sehr_sicherer_geheimer_schlüssel
DATABASE_URL=sqlite:///robocam.db
ESP32CAM_IP=192.168.1.100
ADMIN_USERNAME=admin
ADMIN_PASSWORD=sicheres_passwort
DEBUG=False
```

## 🔧 API-Referenz

### WebSocket Events (ESP32)

#### Eingehende Nachrichten
- **String-Kommandos**: `"up"`, `"down"`, `"left"`, `"right"`, `"center"`, etc.
- **JSON-Kommandos**: `{"x": 90, "y": 45}` für direkte Positionierung
- **Abfragen**: `"get_pos"`, `"get_limits"`, `"client_count"`

#### Ausgehende Nachrichten
- **Binärdaten**: JPEG-Frames für Video-Stream
- **JSON-Responses**: Positionsdaten, Limits, Client-Anzahl

### Flask-Routen

| Route | Methode | Beschreibung | Auth |
|-------|---------|--------------|------|
| `/` | GET | Hauptseite mit Kamera-Interface | ✅ |
| `/login` | GET/POST | Login-Formular | ❌ |
| `/logout` | GET | Benutzer abmelden | ✅ |
| `/health` | GET | Server-Gesundheitsstatus | ❌ |

## 🛠️ Entwicklung

### Build-System

```bash
# ESP32 kompilieren und flashen
cd ESP32Cam
make flash

# Nur kompilieren
make build

# Clean build
make clean

# Serial Monitor
make monitor
```

### Debugging

```bash
# ESP32 Serial Output
make monitor

# Flask Debug-Modus
export FLASK_DEBUG=1
python app.py
```

## 🐛 Troubleshooting

### Häufige Probleme

1. **Kamera startet nicht**
   - Stromversorgung prüfen (min. 5V/1A)
   - WiFi-Credentials in config.h überprüfen
   - Serial Monitor für Fehlermeldungen

2. **Servos bewegen sich nicht**
   - Pin-Konfiguration in config.h überprüfen
   - Servo-Stromversorgung kontrollieren
   - Positionslimits in config.h anpassen

3. **WebSocket-Verbindung fehlschlägt**
   - ESP32 IP-Adresse korrekt?
   - Firewall-Einstellungen prüfen
   - Client-Limit (MAX_CLIENTS) erhöhen

4. **Proxy-Server startet nicht**
   - Python-Dependencies installiert?
   - .env-Datei korrekt konfiguriert?
   - Port 5000 bereits belegt?

### Debug-Ausgaben

ESP32 Serial Monitor zeigt:
- WiFi-Verbindungsstatus
- Client-Verbindungen/Trennungen
- Empfangene WebSocket-Nachrichten
- Kamera-Status

## 📊 Performance

### Optimierungen

- **Frame-Rate**: Anpassbar über `FRAME_INTERVAL` (default: 100ms)
- **Bildqualität**: Konfigurierbar in Camera-Klasse
- **Client-Limit**: Über `MAX_CLIENTS` einstellbar
- **CPU-Frequenz**: Optional auf 240MHz erhöhbar

### Monitoring

- **Client-Anzahl**: Real-time im Web-Interface
- **Frame-Rate**: Über Serial Monitor
- **Memory Usage**: PlatformIO Monitor

## 🤝 Contributing

1. Fork des Repositories
2. Feature-Branch erstellen (`git checkout -b feature/amazing-feature`)
3. Änderungen committen (`git commit -m 'Add amazing feature'`)
4. Branch pushen (`git push origin feature/amazing-feature`)
5. Pull Request erstellen

## 📄 Lizenz

Dieses Projekt ist unter der MIT-Lizenz lizenziert - siehe [LICENSE](LICENSE) für Details.

## 🙏 Danksagungen

- ESP32-CAM Community
- AsyncWebServer Library
- Flask Framework
- PlatformIO Team

## 📞 Support

Bei Fragen oder Problemen:
- GitHub Issues öffnen
- [Wiki](../../wiki) durchsuchen
- Diskussionen im [Forum](../../discussions)
