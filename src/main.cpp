#include <Arduino.h>

#include "esp_camera.h"

#include "ServoService.hpp"
#include "CamController.hpp"

ServoService servoX(14);
ServoService servoY(15);

CamController camController;


void setup() {
  Serial.begin(115200);

  servoX.begin();
  servoY.begin();

  camController.begin(&servoX, &servoY);
}


void loop() {
  servoX.run();
  servoY.run();
  camController.run();
}
