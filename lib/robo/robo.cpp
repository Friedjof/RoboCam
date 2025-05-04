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

void Robo::init() {
    servo1.setPeriodHertz(50);
    servo1.attach(GPIO_SERVO_1, 500, 2400);
    
    servo2.setPeriodHertz(50);
    servo2.attach(GPIO_SERVO_2, 500, 2400);
    
    setPosition(START_POS_X, START_POS_Y);
    
    Serial.println("Servos initialized");
}

// Setzen der Servo-Positionen
void Robo::setPosition(int x, int y) {
    // Begrenze Werte auf gültigen Bereich
    x = constrain(x, MIN_POS_X, MAX_POS_X);
    y = constrain(y, MIN_POS_Y, MAX_POS_Y);
    
    // Aktuelle Position speichern
    goalX = x;
    goalY = y;
    
    Serial.printf("Servo position set to X: %d, Y: %d\n", x, y);
}

void Robo::loop() {
    if (millis() - lastUpdateTime > interval && (posX != goalX || posY != goalY)) {
        // Easing-Formel: Neuer Wert ist ein Stück näher am Ziel
        float newX = posX + (goalX - posX) * easing;
        float newY = posY + (goalY - posY) * easing;

        // Wenn nahe genug am Ziel, direkt auf Ziel setzen
        if (abs(newX - goalX) < 1.0f) newX = goalX;
        if (abs(newY - goalY) < 1.0f) newY = goalY;

        servo1.write((int)newX);
        servo2.write((int)newY);

        posX = (int)newX;
        posY = (int)newY;

        lastUpdateTime = millis();
    }
}