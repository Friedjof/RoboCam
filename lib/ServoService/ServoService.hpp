#ifndef SERVO_SERVICE_HPP
#define SERVO_SERVICE_HPP

#include <Arduino.h>
#include <ESP32Servo.h>


class ServoService {
public:
  ServoService(int pin);

  void begin();
  void run();

  void setPosition(int angle);
private:
  Servo servo;
  uint8_t pin;
  int angle = 0;
};

#endif // SERVO_SERVICE_HPP