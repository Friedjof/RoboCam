#include <Arduino.h>
#include <ESP32Servo.h>

#include "../../include/config.h"


class Robo {
private:
    Servo servo1;
    Servo servo2;

    int posX;
    int posY;
  
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
};