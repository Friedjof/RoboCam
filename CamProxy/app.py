from datetime import datetime
import threading
import websocket
import base64
import time
import signal

from decouple import config

from flask import Flask, render_template, request, redirect, session
from flask_socketio import SocketIO, emit

from models import db, User


app = Flask(__name__)
app.config['SECRET_KEY'] = config("SECRET_KEY", "default_secret_key")


app.config['SQLALCHEMY_DATABASE_URI'] = config("DATABASE_URL", "sqlite:///sqlite.db")
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False


db.init_app(app)
socketio = SocketIO(app)

esp32cam_ip = config("ESP32CAM_IP")
esp32cam_ws_url = f"ws://{esp32cam_ip}/ws"
latest_frame = None
connected_clients = 0

ws_cam = None  # Globales Objekt für die WebSocket-Verbindung

def esp32cam_listener():
    global ws_cam
    def on_message(ws, message):
        global latest_frame
        if isinstance(message, bytes):
            b64 = base64.b64encode(message).decode('utf-8')
            latest_frame = b64  # Frame speichern
            socketio.emit('cam_data', b64)
        else:
            latest_frame = message  # Falls kein Bild, trotzdem speichern
            socketio.emit('cam_data', message)

    def on_error(ws, error):
        print("ESP32CAM WebSocket Fehler:", error)
        ws.close()

    def on_close(ws, close_status_code, close_msg):
        print("ESP32CAM WebSocket Verbindung geschlossen")

    def on_open(ws):
        print("ESP32CAM WebSocket verbunden")
        def send_frames():
            while ws.keep_running:
                if connected_clients > 0:
                    try:
                        ws.send('getframe')
                        time.sleep(.1)
                    except Exception as e:
                        print("Fehler beim Senden von getframe:", e)
                        break
                else:
                    time.sleep(1)
        threading.Thread(target=send_frames, daemon=True).start()

    ws_cam = websocket.WebSocketApp(
        esp32cam_ws_url,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close,
        on_open=on_open
    )
    ws_cam.run_forever()

def cleanup(signum, frame):
    global ws_cam
    print("Beende Flask, schließe WebSocket zur Kamera...")
    if ws_cam:
        try:
            ws_cam.keep_running = False
            ws_cam.close()
        except Exception as e:
            print("Fehler beim Schließen der WebSocket:", e)
    exit(0)

# Signal-Handler registrieren
signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)

threading.Thread(target=esp32cam_listener, daemon=True).start()


@app.route('/', methods=['GET', 'POST'])
def index():
    if 'username' in session:
        username = session['username']
        return render_template('index.html', username=username)
    return redirect('/login')


@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        if 'username' in request.form and 'password' in request.form:
            username = request.form['username']
            password = request.form['password']
            
            user = User.query.filter_by(username=username).first()
            if user and user.check_password(password):
                session['username'] = username
                return redirect('/')
            else:
                return render_template('login.html', error='Invalid credentials')
        else:
            return render_template('login.html', error='Bitte Benutzername und Passwort angeben.')
    return render_template('login.html')


@app.route('/logout', methods=['GET'])
def logout():
    session.pop('username', None)
    return redirect('/login')


@app.context_processor
def inject_current_year():
    return {'current_year': datetime.now().year}


@socketio.on('getframe')
def handle_get_frame():
    global latest_frame
    if latest_frame:
        emit('cam_data', latest_frame)
    else:
        emit('cam_data', None)


# Flask-SocketIO-Event für Browser-Clients
@socketio.on('connect')
def handle_connect():
    global connected_clients
    connected_clients += 1
    print(f"Browser-Client verbunden ({connected_clients} verbunden)")


@socketio.on('disconnect')
def handle_disconnect():
    global connected_clients
    connected_clients = max(0, connected_clients - 1)
    print(f"Browser-Client getrennt ({connected_clients} verbunden)")


if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    socketio.run(app, debug=True)
