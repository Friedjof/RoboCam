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
  int getAngle();

  static constexpr int MIN_ANGLE = 0;
  static constexpr int MAX_ANGLE = 180;
  static constexpr int DEFAULT_ANGLE = 90;

private:
  Servo servo;
  uint8_t pin;
  int angle = DEFAULT_ANGLE;
};

#endif // SERVO_SERVICE_HPP
