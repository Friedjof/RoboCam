#include "ServoService.hpp"

ServoService::ServoService(int pin) : pin(pin) {}

void ServoService::begin() {
    servo.attach(pin);
    servo.write(angle);
}

void ServoService::run() {
    // Currently no periodic tasks are needed for the servo
}

void ServoService::setPosition(int angle) {
    this->angle = angle;
    servo.write(angle);
}
