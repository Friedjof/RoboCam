#ifndef WEB_SERVICE_HPP
#define WEB_SERVICE_HPP

#include <Arduino.h>
#include <WebServer.h>
#include <vector>
#include "esp_http_server.h"

#include "CamController.hpp"
#include "ServoService.hpp"
#include "CONFIG.hpp"

class WebService {
public:
  struct QualityPreset {
    const char* id;
    framesize_t frameSize;
    uint8_t jpegQuality;
    const char* label;
  };

  explicit WebService(uint16_t port = API_PORT);

  void begin(CamController* camController, ServoService* servoX, ServoService* servoY);
  void handle();
  bool isAutoTrackingEnabled() const;

private:
  static constexpr int SERVO_MANUAL_DELTA_LIMIT = 15;

  void registerRoutes();
  void handleRoot();
  void handleStatus();
  void handlePhoto();
  void handleModeGet();
  void handleModeUpdate();
  void handleQualityGet();
  void handleQualityUpdate();
  void handleServoMove();
  void startStreamServer();
  String frameSizeLabel(framesize_t size) const;

  const QualityPreset* findPreset(const String &id) const;
  const QualityPreset& currentPreset() const;
  void applyQuality(const QualityPreset &preset);
  void applyQualityValue(uint8_t qualityValue);

  WebServer server;
  httpd_handle_t streamServer = nullptr;
  CamController* camController = nullptr;
  ServoService* servoX = nullptr;
  ServoService* servoY = nullptr;

  bool autoTrackingEnabled = false;
  String currentQualityPreset = "high";
  uint8_t currentQualityValue = 12;
  framesize_t currentFrameSize = FRAMESIZE_VGA;
};

#endif // WEB_SERVICE_HPP
