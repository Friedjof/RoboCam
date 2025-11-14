#include <Arduino.h>
#include <WiFi.h>

#include "ServoService.hpp"
#include "CamController.hpp"
#include "WebService.hpp"
#include "CONFIG.hpp"

ServoService servoX(12);
ServoService servoY(13);

CamController camController;
WebService webService;

void startAccessPoint();

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Starting setup...");

  servoX.begin();
  servoY.begin();

  camController.begin(&servoX, &servoY);

  startAccessPoint();
  webService.begin(&camController, &servoX, &servoY);

  Serial.println("Setup complete.");
}

void loop() {
  if (webService.isAutoTrackingEnabled()) {
    camController.runAutoTracking();
  }
  webService.handle();
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  const bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);
  if (!apStarted) {
    Serial.println("Failed to start access point");
    return;
  }

  Serial.printf("Access point \"%s\" started\n", AP_SSID);
  Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
}
