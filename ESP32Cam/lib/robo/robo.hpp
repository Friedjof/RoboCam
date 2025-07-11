#include <Arduino.h>
#include <ESP32Servo.h>

#include "../../include/config.h"


class Robo {
private:
    Servo servo1;
    Servo servo2;

    int posX = START_POS_X;
    int posY = START_POS_Y;

    int goalX = START_POS_X;
    int goalY = START_POS_Y;

    uint32_t lastUpdateTime = 0;

    uint8_t updateSteps = 2;

    const unsigned long interval = 60;

    const float easing = 0.2f;

public:
    Robo();
    ~Robo();

    // Initialisierung der Servos
    void init();

    // Setzen der Servo-Positionen
    void setPosition(int x, int y);

    // Getter für aktuelle Positionen
    int getPositionX() { return posX; }
    int getPositionY() { return posY; }

    void loop();
};