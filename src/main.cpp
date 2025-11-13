#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <vector>

#include "ServoService.hpp"
#include "CamController.hpp"
#include "CONFIG.hpp"

ServoService servoX(14);
ServoService servoY(15);

CamController camController;
WebServer photoServer(API_PORT);

void startAccessPoint();
void setupPhotoApi();
void handlePhotoRequest();
void handleStatusRequest();

void setup() {
  Serial.begin(115200);
  delay(100); // allow UART bridge to settle
  Serial.println("Starting setup...");

  servoX.begin();
  servoY.begin();

  camController.begin(&servoX, &servoY);

  startAccessPoint();
  setupPhotoApi();

  Serial.println("Setup complete.");
}

void loop() {
  servoX.run();
  servoY.run();
  camController.run();
  photoServer.handleClient();
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

void setupPhotoApi() {
  photoServer.on("/", HTTP_GET, handleStatusRequest);
  photoServer.on("/photo", HTTP_GET, handlePhotoRequest);
  photoServer.onNotFound([]() {
    photoServer.send(404, "text/plain", "Endpoint not found");
  });
  photoServer.begin();
  Serial.printf("Photo API ready at http://%s:%d/photo\n", WiFi.softAPIP().toString().c_str(), API_PORT);
}

void handleStatusRequest() {
  const String payload = String("{\"status\":\"ok\",\"ip\":\"") + WiFi.softAPIP().toString() + "\"}";
  photoServer.sendHeader("Cache-Control", "no-store");
  photoServer.send(200, "application/json", payload);
}

void handlePhotoRequest() {
  std::vector<uint8_t> jpegBuffer;
  if (!camController.captureRotatedJpeg(jpegBuffer)) {
    photoServer.send(503, "text/plain", "Unable to capture rotated frame");
    return;
  }

  photoServer.sendHeader("Cache-Control", "no-store");
  photoServer.sendHeader("Access-Control-Allow-Origin", "*");
  photoServer.setContentLength(jpegBuffer.size());
  photoServer.send_P(
    200,
    "image/jpeg",
    reinterpret_cast<const char*>(jpegBuffer.data()),
    jpegBuffer.size()
  );
}
