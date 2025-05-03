#include "robo.hpp"

// Konstruktor
Robo::Robo() : posX(90), posY(90) {
    // Initialisierung mit Mittelwerten (90 Grad)
}

// Destruktor
Robo::~Robo() {
    // Ressourcen freigeben
    servo1.detach();
    servo2.detach();
}

// Initialisierung der Servos
void Robo::init() {
    // ESP32PWM::allocateTimer(0);  // Optional: Timer zuweisen
    // ESP32PWM::allocateTimer(1);
    
    // Servos an GPIO-Pins anschließen
    servo1.setPeriodHertz(50);      // Standard-PWM-Frequenz für Servos
    servo1.attach(GPIO_SERVO_1, 500, 2400); // Min und Max Pulsdauer in Mikrosekunden
    
    servo2.setPeriodHertz(50);
    servo2.attach(GPIO_SERVO_2, 500, 2400);
    
    // Startposition einstellen
    setPosition(posX, posY);
    
    Serial.println("Servos initialized");
}

// Setzen der Servo-Positionen
void Robo::setPosition(int x, int y) {
    // Begrenze Werte auf gültigen Bereich
    x = constrain(x, MIN_POS_X, MAX_POS_X);
    y = constrain(y, MIN_POS_Y, MAX_POS_Y);
    
    // Servos auf neue Position setzen
    servo1.write(x);
    servo2.write(y);
    
    // Aktuelle Position speichern
    posX = x;
    posY = y;
    
    Serial.printf("Servo position set to X: %d, Y: %d\n", x, y);
}