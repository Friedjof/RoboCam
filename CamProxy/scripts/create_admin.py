import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from models import User, db
from flask import Flask
from dotenv import load_dotenv
from decouple import config

# .env laden (optional, falls du python-dotenv nutzt)
load_dotenv()

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = config("DATABASE_URL", "sqlite:///sqlite.db")
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db.init_app(app)

ADMIN_USERNAME = config("ADMIN_USERNAME", "admin")
ADMIN_PASSWORD = config("ADMIN_PASSWORD", "admin")

with app.app_context():
    db.create_all()
    # Prüfen, ob Admin schon existiert
    admin = User.query.filter_by(username=ADMIN_USERNAME).first()
    if not admin:
        admin = User(username=ADMIN_USERNAME, password=ADMIN_PASSWORD)
        admin.is_admin = True
        db.session.add(admin)
        db.session.commit()
        print(f"Admin-Benutzer '{ADMIN_USERNAME}' wurde angelegt.")
    else:
        print(f"Admin-Benutzer '{ADMIN_USERNAME}' existiert bereits.")