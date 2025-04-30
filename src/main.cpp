#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>

#include "esp_camera.h"

#define START_POS_X 0
#define MIN_POS_X 0
#define MAX_POS_X 90

#define START_POS_Y 0
#define MIN_POS_Y 0
#define MAX_POS_Y 360

#define SERVO_1_PIN 2
#define SERVO_2_PIN 14

#define TOKEN "H264GG3zPrmf8wPJahBD5Ui9"


Servo servo1;
Servo servo2;


// current position of the servos
int pos_x = 0;
int pos_y = 0;


void setup() {
  Serial.begin(115200);
  delay(1000);

  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);

  servo1.attach(SERVO_1_PIN, 500, 2500);
  servo2.attach(SERVO_2_PIN, 500, 2500);

  // Set initial position
  servo1.write(START_POS_X);
  servo2.write(START_POS_Y);
}

void loop() {

}
