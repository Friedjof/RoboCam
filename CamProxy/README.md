# CamProxy Server 🌐

Flask-basierter Proxy-Server für das RoboCam-System mit Benutzer-Authentifizierung und erweiterten Features.

## 🎯 Features

- **WebSocket-Proxy** zwischen Browser und ESP32
- **Benutzer-Authentifizierung** mit Session-Management
- **Multi-User-Support** mit Admin-Rechten
- **Live-Video-Streaming** über SocketIO
- **Automatische Frame-Anfrage** basierend auf verbundenen Clients
- **SQLite-Datenbank** für Benutzerverwaltung
- **Responsive Web-Interface**
- **Health-Check-Endpoint**

## 🏗️ Systemanforderungen

- **Python 3.8+**
- **ESP32Cam** im gleichen Netzwerk
- **Moderne Browser** mit WebSocket-Support

## 🚀 Installation

### 1. Python-Umgebung einrichten

```bash
# Repository klonen
git clone https://github.com/yourusername/RoboCam.git
cd RoboCam/CamProxy

# Virtual Environment erstellen
python -m venv venv

# Virtual Environment aktivieren
source venv/bin/activate  # Linux/Mac
# oder: venv\Scripts\activate  # Windows

# Dependencies installieren
pip install -r requirements.txt
```

### 2. Konfiguration

Erstellen Sie die Umgebungsvariablen-Datei:

```bash
cp .env.example .env
```

Bearbeiten Sie `.env`:

```bash
# Flask-Konfiguration
SECRET_KEY=ihr_sehr_sicherer_geheimer_schlüssel_hier
DEBUG=False
HOST=0.0.0.0
PORT=5000

# Datenbank
DATABASE_URL=sqlite:///robocam.db

# ESP32Cam-Verbindung
ESP32CAM_IP=192.168.1.100
ESP32CAM_PORT=80
ESP32CAM_PATH=/ws

# Admin-Account (für Erstsetup)
ADMIN_USERNAME=admin
ADMIN_PASSWORD=sicheres_admin_passwort

# Logging
LOG_LEVEL=INFO
LOG_FILE=robocam.log
```

### 3. Datenbank initialisieren

```bash
# Datenbank-Tabellen erstellen
python -c "from app import create_app; from models import db; app = create_app(); app.app_context().push(); db.create_all()"

# Admin-Benutzer erstellen
python scripts/create_admin.py
```

## 🎮 Verwendung

### Server starten

```bash
# Entwicklungsmodus
python app.py

# Produktionsmodus mit Gunicorn
gunicorn -w 4 -k eventlet --bind 0.0.0.0:5000 app:app
```

### Web-Interface

1. **Browser öffnen**: `http://localhost:5000`
2. **Mit Admin-Credentials anmelden**
3. **Live-Video-Stream** und Steuerung verwenden

## 🔧 API-Referenz

### HTTP-Routen

| Route | Methode | Beschreibung | Auth | Rückgabe |
|-------|---------|--------------|------|----------|
| `/` | GET | Hauptseite mit Kamera-Interface | ✅ | HTML-Seite |
| `/login` | GET | Login-Formular anzeigen | ❌ | HTML-Formular |
| `/login` | POST | Login-Daten verarbeiten | ❌ | Redirect/Error |
| `/logout` | GET | Benutzer abmelden | ✅ | Redirect zu Login |
| `/health` | GET | Server-Gesundheitsstatus | ❌ | JSON-Status |

### SocketIO-Events

#### Client → Server

| Event | Beschreibung | Parameter | Beispiel |
|-------|--------------|-----------|----------|
| `connect` | Client-Verbindung | - | Automatisch |
| `disconnect` | Client-Trennung | - | Automatisch |
| `servo_command` | Servo-Steuerung | `{command: string}` | `{command: "up"}` |
| `position_command` | Direkte Positionierung | `{x: int, y: int}` | `{x: 90, y: 45}` |
| `get_position` | Position abrufen | - | - |
| `light_command` | LED-Steuerung | `{state: boolean}` | `{state: true}` |

#### Server → Client

| Event | Beschreibung | Daten | Beispiel |
|-------|--------------|-------|----------|
| `video_frame` | Video-Frame | Base64-kodiertes JPEG | - |
| `position_update` | Positions-Update | `{x: int, y: int}` | `{x: 90, y: 45}` |
| `client_count` | Anzahl Clients | `{count: int}` | `{count: 3}` |
| `error` | Fehlermeldung | `{message: string}` | `{message: "ESP32 nicht erreichbar"}` |

## 📁 Projektstruktur

```
CamProxy/
├── app.py                    # Hauptanwendung (Flask + SocketIO)
├── models.py                 # Datenbankmodelle (User)
├── requirements.txt          # Python-Dependencies
├── .env.example             # Umgebungsvariablen-Vorlage
├── scripts/
│   ├── create_admin.py      # Admin-Benutzer erstellen
│   └── backup_db.py         # Datenbank-Backup
├── templates/
│   ├── base.html            # Basis-Template
│   ├── login.html           # Login-Seite
│   └── index.html           # Hauptseite mit Kamera
├── static/
│   ├── css/
│   │   └── style.css        # Stylesheet
│   ├── js/
│   │   └── scripts.js       # JavaScript für SocketIO
│   └── images/
│       └── favicon.ico      # App-Icon
└── logs/                    # Log-Dateien (auto-erstellt)
```

## 🔒 Benutzer-Management

### Neuen Benutzer erstellen

```python
from models import User, db
from app import create_app

app = create_app()
with app.app_context():
    user = User(username='newuser', password='password123')
    user.is_active = True
    db.session.add(user)
    db.session.commit()
```

### Benutzer verwalten

```python
# Benutzer deaktivieren
user = User.query.filter_by(username='username').first()
user.is_active = False
db.session.commit()

# Admin-Rechte vergeben
user.is_admin = True
db.session.commit()

# Passwort ändern
user.set_password('new_password')
db.session.commit()
```

### CLI-Tool (create_admin.py)

```bash
python scripts/create_admin.py
# Folgen Sie den Eingabeaufforderungen
```

## 🌐 WebSocket-Proxy

### Funktionsweise

```python
# ESP32-Verbindung
esp32_ws = None

async def connect_to_esp32():
    global esp32_ws
    try:
        esp32_ws = await websockets.connect(f"ws://{ESP32CAM_IP}/ws")
        print(f"Connected to ESP32 at {ESP32CAM_IP}")
    except Exception as e:
        print(f"Failed to connect to ESP32: {e}")

# Proxy-Funktionen
async def proxy_to_esp32(message):
    if esp32_ws:
        await esp32_ws.send(message)

async def proxy_from_esp32():
    async for message in esp32_ws:
        socketio.emit('video_frame', message, broadcast=True)
```

### Automatische Frame-Anfrage

```python
@socketio.on('connect')
def handle_connect():
    global connected_clients
    connected_clients += 1
    
    # Frame-Anfrage starten wenn erster Client
    if connected_clients == 1:
        start_frame_requests()

def start_frame_requests():
    def request_frames():
        while connected_clients > 0:
            if esp32_ws:
                asyncio.run(esp32_ws.send('getframe'))
            time.sleep(0.1)  # 10 FPS
    
    threading.Thread(target=request_frames, daemon=True).start()
```

## 🔧 Konfiguration

### Flask-Settings

```python
# In app.py
class Config:
    SECRET_KEY = os.getenv('SECRET_KEY')
    SQLALCHEMY_DATABASE_URI = os.getenv('DATABASE_URL')
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    
    # SocketIO Settings
    SOCKETIO_PING_TIMEOUT = 60
    SOCKETIO_PING_INTERVAL = 25
    
    # Session Settings
    PERMANENT_SESSION_LIFETIME = timedelta(hours=24)
```

### ESP32-Verbindung

```python
# Verbindungsparameter
ESP32CAM_IP = os.getenv('ESP32CAM_IP', '192.168.1.100')
ESP32CAM_PORT = int(os.getenv('ESP32CAM_PORT', 80))
ESP32CAM_PATH = os.getenv('ESP32CAM_PATH', '/ws')

# Retry-Logik
RECONNECT_DELAY = 5  # Sekunden
MAX_RECONNECT_ATTEMPTS = 10
```

### Logging

```python
import logging

logging.basicConfig(
    level=getattr(logging, os.getenv('LOG_LEVEL', 'INFO')),
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(os.getenv('LOG_FILE', 'robocam.log')),
        logging.StreamHandler()
    ]
)
```

## 🐛 Debugging

### Log-Ausgaben

```bash
# Live-Logs anzeigen
tail -f robocam.log

# Flask Debug-Modus
export FLASK_DEBUG=1
python app.py
```

### Häufige Probleme

1. **ESP32 nicht erreichbar**
   ```bash
   # IP-Adresse testen
   ping 192.168.1.100
   
   # Port-Verfügbarkeit prüfen
   telnet 192.168.1.100 80
   ```

2. **Datenbank-Fehler**
   ```bash
   # Datenbank neu erstellen
   rm robocam.db
   python -c "from app import create_app; from models import db; app = create_app(); app.app_context().push(); db.create_all()"
   ```

3. **SocketIO-Verbindungsfehler**
   ```javascript
   // Browser-Konsole
   socket.on('connect_error', (error) => {
       console.log('Connection Error:', error);
   });
   ```

4. **Session-Probleme**
   ```python
   # Secret Key prüfen
   print(app.config['SECRET_KEY'])
   
   # Session-Daten löschen
   session.clear()
   ```

## 📊 Monitoring

### Health-Check

```bash
curl http://localhost:5000/health
# Antwort: {"status": "healthy", "esp32_connected": true, "clients": 2}
```

### Metriken sammeln

```python
# Custom Middleware für Metriken
@app.before_request
def before_request():
    g.start_time = time.time()

@app.after_request
def after_request(response):
    response_time = time.time() - g.start_time
    logging.info(f"Request: {request.method} {request.path} - {response.status_code} - {response_time:.3f}s")
    return response
```

## 🚀 Produktions-Deployment

### Gunicorn-Konfiguration

```bash
# gunicorn.conf.py
bind = "0.0.0.0:5000"
workers = 4
worker_class = "eventlet"
worker_connections = 1000
timeout = 30
keepalive = 2
max_requests = 1000
max_requests_jitter = 100
```

### Systemd-Service

```ini
# /etc/systemd/system/robocam.service
[Unit]
Description=RoboCam Proxy Server
After=network.target

[Service]
Type=simple
User=robocam
WorkingDirectory=/home/robocam/RoboCam/CamProxy
Environment=PATH=/home/robocam/RoboCam/CamProxy/venv/bin
ExecStart=/home/robocam/RoboCam/CamProxy/venv/bin/gunicorn -c gunicorn.conf.py app:app
Restart=always

[Install]
WantedBy=multi-user.target
```

### Nginx-Proxy

```nginx
# /etc/nginx/sites-available/robocam
server {
    listen 80;
    server_name your-domain.com;
    
    location / {
        proxy_pass http://127.0.0.1:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
    
    location /socket.io/ {
        proxy_pass http://127.0.0.1:5000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

## 🔐 Sicherheit

### HTTPS aktivieren

```python
# SSL-Kontext
context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
context.load_cert_chain('cert.pem', 'key.pem')

socketio.run(app, host='0.0.0.0', port=5000, ssl_context=context)
```

### Rate Limiting

```python
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

limiter = Limiter(
    app,
    key_func=get_remote_address,
    default_limits=["200 per day", "50 per hour"]
)

@app.route('/login', methods=['POST'])
@limiter.limit("5 per minute")
def login():
    # Login-Logik
```

## 🤝 Entwicklung

### Neue Features hinzufügen

1. **Route** in `app.py` definieren
2. **Template** in `templates/` erstellen
3. **SocketIO-Event** für Real-time-Features
4. **Tests** schreiben

### Testing

```bash
# Unit Tests
python -m pytest tests/

# Integration Tests
python -m pytest tests/integration/

# Coverage Report
python -m pytest --cov=app tests/
```

---

**CamProxy Server** - Teil des RoboCam-Projekts