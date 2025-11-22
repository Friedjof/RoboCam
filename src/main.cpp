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
esp_err_t streamHandler(httpd_req_t *req);


uint64_t faceDetectionTime = 0;
static const char STREAM_CONTENT_TYPE[] = "multipart/x-mixed-replace; boundary=frame";

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Starting setup...");

  servoX.begin();
  servoY.begin();

  camController.begin(&servoX, &servoY);

  startAccessPoint();
  webService.setStreamHandler(streamHandler); // set before begin so stream server can start
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

esp_err_t streamHandler(httpd_req_t *req) {
  auto *service = static_cast<WebService *>(req->user_ctx);
  if (!service) {
    return ESP_FAIL;
  }

  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char partBuffer[64];
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("stream: capture failed");
      res = ESP_FAIL;
      break;
    }

    if (millis() - faceDetectionTime > 1000) {
      FaceBox box = {};

      camController.getFaceBox(fb, &box);

      camController.storeFaceBox(
        box,
        static_cast<uint16_t>(fb->width),
        static_cast<uint16_t>(fb->height)
      );

      Serial.printf(
        "Face box: x=%d, y=%d, w=%d, h=%d, detected=%s\n",
        box.x,
        box.y,
        box.width,
        box.height,
        box.detected ? "true" : "false"
      );

      faceDetectionTime = millis();
    }

    const size_t headerLen = snprintf(
      partBuffer,
      sizeof(partBuffer),
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      static_cast<unsigned>(fb->len)
    );

    res = httpd_resp_send_chunk(req, partBuffer, headerLen);
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(
        req,
        reinterpret_cast<const char *>(fb->buf),
        fb->len
      );
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "\r\n", 2);
    }

    esp_camera_fb_return(fb);

    if (res != ESP_OK) {
      break;
    }
  }

  return res;
}
