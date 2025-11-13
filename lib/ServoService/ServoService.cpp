#include "ServoService.hpp"

ServoService::ServoService(int pin) : pin(pin) {}

void ServoService::begin() {
    servo.attach(pin);
    const int clampedAngle = constrain(angle, MIN_ANGLE, MAX_ANGLE);
    angle = clampedAngle;
    servo.write(clampedAngle);
}

void ServoService::run() {
    // Currently no periodic tasks are needed for the servo
}

void ServoService::setPosition(int angle) {
    const int clampedAngle = constrain(angle, MIN_ANGLE, MAX_ANGLE);
    this->angle = clampedAngle;
    servo.write(clampedAngle);
}

int ServoService::getAngle() {
    return this->angle;
}
