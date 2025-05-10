from datetime import datetime
from uuid import uuid4

from sqlalchemy import Column, String, DateTime, Boolean
from sqlalchemy.ext.declarative import declarative_base
from flask_sqlalchemy import SQLAlchemy

import bcrypt


db = SQLAlchemy()


class User(db.Model):
    __tablename__ = 'users'
    id = Column(String(36), primary_key=True, default=lambda: f"{uuid4()}")
    username = Column(String(50), unique=True, nullable=False)
    password = Column(String(100), nullable=False)

    created_at = Column(DateTime, nullable=False)
    updated_at = Column(DateTime, nullable=False)

    active = Column(Boolean, default=True)
    is_admin = Column(Boolean, default=False)

    def __init__(self, username: str, password: str) -> None:
        self.username = username
        self.password = bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt()).decode('utf-8')
        self.created_at = datetime.now()
        self._update()

    def check_password(self, password: str) -> bool:
        return bcrypt.checkpw(password.encode('utf-8'), self.password.encode('utf-8'))

    def update_password(self, old_password: str, new_password: str) -> bool:
        if self.check_password(old_password):
            self.password = bcrypt.hashpw(new_password.encode('utf-8'), bcrypt.gensalt()).decode('utf-8')
            self._update()
            return True
        return False

    def activate(self) -> None:
        self.active = True
        self._update()
    
    def deactivate(self) -> None:
        self.active = False
        self._update()
    
    def is_admin(self) -> bool:
        return self.is_admin

    def _update(self) -> None:
        self.updated_at = datetime.now()

    def __repr__(self) -> str:
        return f"<User(username={self.username})>"
