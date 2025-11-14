#include "WebService.hpp"

#include <WiFi.h>
#include <vector>

#include "web_ui_gz.h"
#include "esp_camera.h"
#include "img_converters.h"

namespace {
const WebService::QualityPreset QUALITY_PRESETS[] = {
  {"low", FRAMESIZE_QVGA, 24, "QVGA (320x240)"},
  {"medium", FRAMESIZE_VGA, 18, "VGA (640x480)"},
  {"high", FRAMESIZE_XGA, 12, "XGA (1024x768)"}
};
constexpr size_t QUALITY_PRESET_COUNT = sizeof(QUALITY_PRESETS) / sizeof(QUALITY_PRESETS[0]);

static const char STREAM_CONTENT_TYPE[] = "multipart/x-mixed-replace; boundary=frame";

esp_err_t streamHandler(httpd_req_t *req) {
  auto* service = static_cast<WebService*>(req->user_ctx);
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
}

WebService::WebService(uint16_t port)
  : server(port) {}

void WebService::begin(CamController* camController, ServoService* servoX, ServoService* servoY) {
  this->camController = camController;
  this->servoX = servoX;
  this->servoY = servoY;

  this->applyQuality(this->currentPreset());
  this->registerRoutes();
  this->server.begin();
  this->startStreamServer();

  const String ip = WiFi.softAPIP().toString();
  Serial.printf("Livestream UI: http://%s:%d/\n", ip.c_str(), API_PORT);
  Serial.printf("Status JSON:   http://%s:%d/status\n", ip.c_str(), API_PORT);
  Serial.printf("Photo endpoint http://%s:%d/photo\n", ip.c_str(), API_PORT);
  Serial.printf("Stream endpoint http://%s:%d/stream\n", ip.c_str(), 81);
}

void WebService::handle() {
  this->server.handleClient();
}

bool WebService::isAutoTrackingEnabled() const {
  return this->autoTrackingEnabled;
}

void WebService::registerRoutes() {
  this->server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  this->server.on("/status", HTTP_GET, [this]() { this->handleStatus(); });
  this->server.on("/photo", HTTP_GET, [this]() { this->handlePhoto(); });
  this->server.on("/stream", HTTP_GET, [this]() {
    const String redirectUrl = String("http://") + WiFi.softAPIP().toString() + ":81/stream";
    this->server.sendHeader("Location", redirectUrl);
    this->server.send(302, "text/plain", "Use MJPEG stream on port 81.");
  });

  this->server.on("/mode", HTTP_GET, [this]() { this->handleModeGet(); });
  this->server.on("/mode", HTTP_POST, [this]() { this->handleModeUpdate(); });
  this->server.on("/mode", HTTP_OPTIONS, [this]() {
    this->server.sendHeader("Access-Control-Allow-Origin", "*");
    this->server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    this->server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    this->server.send(204);
  });

  this->server.on("/quality", HTTP_GET, [this]() { this->handleQualityGet(); });
  this->server.on("/quality", HTTP_POST, [this]() { this->handleQualityUpdate(); });
  this->server.on("/quality", HTTP_OPTIONS, [this]() {
    this->server.sendHeader("Access-Control-Allow-Origin", "*");
    this->server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    this->server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    this->server.send(204);
  });

  this->server.on("/servo/move", HTTP_POST, [this]() { this->handleServoMove(); });
  this->server.on("/servo/move", HTTP_OPTIONS, [this]() {
    this->server.sendHeader("Access-Control-Allow-Origin", "*");
    this->server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    this->server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    this->server.send(204);
  });

  this->server.onNotFound([this]() {
    this->server.send(404, "text/plain", "Endpoint not found");
  });
}

void WebService::handleRoot() {
  this->server.sendHeader("Cache-Control", "no-store, must-revalidate");
  this->server.sendHeader("Content-Encoding", "gzip");
  this->server.send_P(
    200,
    "text/html",
    reinterpret_cast<const char*>(WEB_UI_HTML_GZ),
    WEB_UI_HTML_GZ_len
  );
}

void WebService::handleStatus() {
  FaceBox box = {};
  uint16_t frameWidth = 0;
  uint16_t frameHeight = 0;
  uint64_t timestamp = 0;
  bool faceDetected = false;
  if (this->camController) {
    faceDetected = this->camController->getLastFaceBox(box, frameWidth, frameHeight, timestamp);
  }

  String payload = String("{\"status\":\"ok\",\"ip\":\"") + WiFi.softAPIP().toString() +
    "\",\"mode\":\"" + (this->autoTrackingEnabled ? "auto" : "manual") + "\"" +
    ",\"quality\":\"" + this->currentQualityPreset + "\"," +
    "\"qualityValue\":" + String(this->currentQualityValue) + "," +
    "\"frameSizeId\":" + String(static_cast<int>(this->currentFrameSize)) + "," +
    "\"frameSizeLabel\":\"" + this->frameSizeLabel(this->currentFrameSize) + "\"," +
    "\"faceDetected\":" + String(faceDetected ? "true" : "false") + ",";

  payload += "\"face\":{";
  payload += "\"x\":" + String(box.x) + ",";
  payload += "\"y\":" + String(box.y) + ",";
  payload += "\"width\":" + String(box.width) + ",";
  payload += "\"height\":" + String(box.height) + ",";
  payload += "\"frameWidth\":" + String(frameWidth) + ",";
  payload += "\"frameHeight\":" + String(frameHeight) + ",";
  payload += "\"timestamp\":" + String(timestamp);
  payload += "}}";

  this->server.sendHeader("Cache-Control", "no-store");
  this->server.send(200, "application/json", payload);
}

void WebService::handlePhoto() {
  std::vector<uint8_t> jpegBuffer;
  if (!this->camController->captureJpegWithFaceBox(jpegBuffer)) {
    this->server.send(503, "text/plain", "Unable to capture frame");
    return;
  }

  this->server.sendHeader("Cache-Control", "no-store");
  this->server.sendHeader("Access-Control-Allow-Origin", "*");
  this->server.setContentLength(jpegBuffer.size());
  this->server.send_P(
    200,
    "image/jpeg",
    reinterpret_cast<const char*>(jpegBuffer.data()),
    jpegBuffer.size()
  );
}

void WebService::handleModeGet() {
  const String payload = String("{\"mode\":\"") + (this->autoTrackingEnabled ? "auto" : "manual") + "\"}";
  this->server.sendHeader("Cache-Control", "no-store");
  this->server.sendHeader("Access-Control-Allow-Origin", "*");
  this->server.send(200, "application/json", payload);
}

void WebService::handleModeUpdate() {
  this->server.sendHeader("Cache-Control", "no-store");
  this->server.sendHeader("Access-Control-Allow-Origin", "*");

  if (!this->server.hasArg("mode")) {
    this->server.send(400, "application/json", "{\"error\":\"mode argument required\"}");
    return;
  }

  String requested = this->server.arg("mode");
  requested.toLowerCase();
  if (requested != "auto" && requested != "manual") {
    this->server.send(400, "application/json", "{\"error\":\"mode must be auto or manual\"}");
    return;
  }

  this->autoTrackingEnabled = (requested == "auto");
  Serial.printf("Tracking mode updated: %s\n", this->autoTrackingEnabled ? "AUTO" : "MANUAL");

  const String payload = String("{\"mode\":\"") + requested + "\"}";
  this->server.send(200, "application/json", payload);
}

void WebService::handleQualityGet() {
  const String payload = String("{\"preset\":\"") + this->currentQualityPreset + "\"," +
    "\"quality\":" + String(this->currentQualityValue) + "," +
    "\"frameSizeId\":" + String(static_cast<int>(this->currentFrameSize)) + "," +
    "\"frameSizeLabel\":\"" + this->frameSizeLabel(this->currentFrameSize) + "\"}";
  this->server.sendHeader("Cache-Control", "no-store");
  this->server.sendHeader("Access-Control-Allow-Origin", "*");
  this->server.send(200, "application/json", payload);
}

void WebService::handleQualityUpdate() {
  this->server.sendHeader("Cache-Control", "no-store");
  this->server.sendHeader("Access-Control-Allow-Origin", "*");

  if (this->server.hasArg("preset")) {
    const String presetId = this->server.arg("preset");
    const QualityPreset* preset = this->findPreset(presetId);
    if (!preset) {
      this->server.send(400, "application/json", "{\"error\":\"preset must be low, medium, or high\"}");
      return;
    }
    this->applyQuality(*preset);
    Serial.printf("Quality preset updated: %s (value=%d)\n", preset->id, preset->jpegQuality);
    const String payload = String("{\"preset\":\"") + preset->id + "\"," +
      "\"quality\":" + String(this->currentQualityValue) + "," +
      "\"frameSizeId\":" + String(static_cast<int>(this->currentFrameSize)) + "," +
      "\"frameSizeLabel\":\"" + preset->label + "\"}";
    this->server.send(200, "application/json", payload);
    return;
  }

  if (this->server.hasArg("value")) {
    const int requested = constrain(this->server.arg("value").toInt(), 10, 63);
    this->applyQualityValue(static_cast<uint8_t>(requested));
    this->currentQualityPreset = "custom";
    Serial.printf("Quality set to %d via slider\n", requested);
    const String payload = String("{\"preset\":\"") + this->currentQualityPreset + "\"," +
      "\"quality\":" + String(this->currentQualityValue) + "," +
      "\"frameSizeId\":" + String(static_cast<int>(this->currentFrameSize)) + "," +
      "\"frameSizeLabel\":\"" + this->frameSizeLabel(this->currentFrameSize) + "\"}";
    this->server.send(200, "application/json", payload);
    return;
  }

  this->server.send(400, "application/json", "{\"error\":\"preset or value argument required\"}");
}

void WebService::handleServoMove() {
  this->server.sendHeader("Cache-Control", "no-store");
  this->server.sendHeader("Access-Control-Allow-Origin", "*");

  if (this->autoTrackingEnabled) {
    this->server.send(409, "application/json", "{\"error\":\"manual control disabled while auto mode is active\"}");
    return;
  }

  if (!this->server.hasArg("axis") || !this->server.hasArg("delta")) {
    this->server.send(400, "application/json", "{\"error\":\"axis and delta arguments required\"}");
    return;
  }

  String axis = this->server.arg("axis");
  axis.toLowerCase();
  ServoService* targetServo = nullptr;
  if (axis == "pan") {
    targetServo = this->servoX;
  } else if (axis == "tilt") {
    targetServo = this->servoY;
  } else {
    this->server.send(400, "application/json", "{\"error\":\"axis must be pan or tilt\"}");
    return;
  }

  int delta = this->server.arg("delta").toInt();
  delta = constrain(delta, -SERVO_MANUAL_DELTA_LIMIT, SERVO_MANUAL_DELTA_LIMIT);

  int newAngle = targetServo->getAngle();
  if (delta != 0) {
    newAngle = constrain(newAngle + delta, ServoService::MIN_ANGLE, ServoService::MAX_ANGLE);
    targetServo->setPosition(newAngle);
  }

  Serial.printf("Manual servo move axis=%s delta=%d -> angle=%d\n", axis.c_str(), delta, newAngle);

  const String payload = String("{\"axis\":\"") + axis + "\",\"angle\":" + newAngle + ",\"delta\":" + delta + "}";
  this->server.send(200, "application/json", payload);
}

const WebService::QualityPreset* WebService::findPreset(const String &id) const {
  for (size_t i = 0; i < QUALITY_PRESET_COUNT; ++i) {
    if (id.equalsIgnoreCase(QUALITY_PRESETS[i].id)) {
      return &QUALITY_PRESETS[i];
    }
  }
  return nullptr;
}

const WebService::QualityPreset& WebService::currentPreset() const {
  const QualityPreset* preset = this->findPreset(this->currentQualityPreset);
  if (preset) {
    return *preset;
  }
  return QUALITY_PRESETS[QUALITY_PRESET_COUNT - 1];
}

void WebService::applyQuality(const QualityPreset &preset) {
  this->currentQualityPreset = preset.id;
  this->currentFrameSize = preset.frameSize;
  if (this->camController) {
    this->camController->setFrameSize(preset.frameSize);
  }
  this->applyQualityValue(preset.jpegQuality);
}

void WebService::applyQualityValue(uint8_t qualityValue) {
  this->currentQualityValue = qualityValue;
  if (this->camController) {
    this->camController->setJpegQuality(qualityValue);
  }
}

String WebService::frameSizeLabel(framesize_t size) const {
  uint16_t width = 0;
  uint16_t height = 0;
  CamController::resolveFrameSize(size, width, height);

  String label;
  if (size == FRAMESIZE_QVGA) {
    label = "QVGA";
  } else if (size == FRAMESIZE_VGA) {
    label = "VGA";
  } else if (size == FRAMESIZE_XGA) {
    label = "XGA";
  } else {
    label = "Custom";
  }

  label += " (" + String(width) + "x" + String(height) + ")";
  return label;
}

void WebService::startStreamServer() {
  if (this->streamServer) {
    httpd_stop(this->streamServer);
    this->streamServer = nullptr;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = 32768;
  config.max_open_sockets = 1;
  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;

  httpd_uri_t streamUri = {
    .uri = "/stream*",
    .method = HTTP_GET,
    .handler = streamHandler,
    .user_ctx = this
  };

  if (httpd_start(&this->streamServer, &config) == ESP_OK) {
    httpd_register_uri_handler(this->streamServer, &streamUri);
  } else {
    Serial.println("Failed to start stream server");
  }
}
