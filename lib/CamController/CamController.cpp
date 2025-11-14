#include "CamController.hpp"
#include "img_converters.h"
#include <algorithm>
#include "esp_timer.h"


CamController::CamController()
  : faceDetectorStage1(0.1F, 0.5F, 10, 0.2F),
    faceDetectorStage2(0.5F, 0.3F, 5) {}

void CamController::begin(ServoService* servoX, ServoService* servoY) {
  this->servoX = servoX;
  this->servoY = servoY;

  camera_config_t cameraConfig = {};
  cameraConfig.ledc_channel = LEDC_CHANNEL_0;
  cameraConfig.ledc_timer = LEDC_TIMER_0;
  cameraConfig.pin_d0 = Y2_GPIO_NUM;
  cameraConfig.pin_d1 = Y3_GPIO_NUM;
  cameraConfig.pin_d2 = Y4_GPIO_NUM;
  cameraConfig.pin_d3 = Y5_GPIO_NUM;
  cameraConfig.pin_d4 = Y6_GPIO_NUM;
  cameraConfig.pin_d5 = Y7_GPIO_NUM;
  cameraConfig.pin_d6 = Y8_GPIO_NUM;
  cameraConfig.pin_d7 = Y9_GPIO_NUM;
  cameraConfig.pin_xclk = XCLK_GPIO_NUM;
  cameraConfig.pin_pclk = PCLK_GPIO_NUM;
  cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
  cameraConfig.pin_href = HREF_GPIO_NUM;
  cameraConfig.pin_sccb_sda = SIOD_GPIO_NUM;
  cameraConfig.pin_sccb_scl = SIOC_GPIO_NUM;
  cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
  cameraConfig.pin_reset = RESET_GPIO_NUM;
  cameraConfig.pixel_format = PIXFORMAT_JPEG; // native JPEG for streaming
  cameraConfig.xclk_freq_hz = 20000000;
  cameraConfig.frame_size = FRAMESIZE_VGA; // 640x480 for better livestream quality
  cameraConfig.jpeg_quality = this->photoJpegQuality;
  cameraConfig.fb_count = 2;
  cameraConfig.fb_location = CAMERA_FB_IN_PSRAM;
  cameraConfig.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor) {
    Serial.println("Unable to obtain camera sensor handle");
    return;
  }

  this->activeSensor = sensor;
  this->currentFrameSize = cameraConfig.frame_size;
  this->applyUltraWideSensorPreset(sensor);
  Serial.printf("Camera ready. Frame size: %d, stream quality: %d\n", cameraConfig.frame_size, this->photoJpegQuality);
}

void CamController::runAutoTracking() {
  if (!this->servoX || !this->servoY) {
    return;
  }

  if (millis() - this->lastFaceDetectTime < FACE_DETECT_INTERVAL) {
    return;
  }
  this->lastFaceDetectTime = millis();

  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("Camera capture failed");
    return;
  }

  std::vector<uint8_t> rgbBuffer;
  camera_fb_t rgbFrame = {};
  if (!this->decodeJpegToRgb565(frame, rgbBuffer, rgbFrame)) {
    Serial.println("Failed to convert frame for tracking");
    esp_camera_fb_return(frame);
    return;
  }

  const int frameWidth = rgbFrame.width;
  const int frameHeight = rgbFrame.height;
  FaceBox faceBox = {};

  this->getFaceBox(&rgbFrame, &faceBox);
  esp_camera_fb_return(frame);

  this->storeFaceBox(faceBox, frameWidth, frameHeight);

  if (!faceBox.detected || frameWidth <= 0 || frameHeight <= 0) {
    return;
  }

  const int faceCenterX = faceBox.x + faceBox.width / 2;
  const int faceCenterY = faceBox.y + faceBox.height / 2;
  const int frameCenterX = frameWidth / 2;
  const int frameCenterY = frameHeight / 2;

  const int errorX = faceCenterX - frameCenterX;
  const int errorY = faceCenterY - frameCenterY;

  const int stepX = constrain(errorX * this->STEP_SCALE / frameWidth, -this->MAX_SERVO_STEP, this->MAX_SERVO_STEP);
  const int stepY = constrain(errorY * this->STEP_SCALE / frameHeight, -this->MAX_SERVO_STEP, this->MAX_SERVO_STEP);

  if (stepX != 0) {
    const int newX = constrain(this->servoX->getAngle() - stepX, ServoService::MIN_ANGLE, ServoService::MAX_ANGLE);
    this->servoX->setPosition(newX);
  }
  if (stepY != 0) {
    const int newY = constrain(this->servoY->getAngle() - stepY, ServoService::MIN_ANGLE, ServoService::MAX_ANGLE);
    this->servoY->setPosition(newY);
  }
}

// Funktion gibt die Position des ersten erkannten Gesichts zurück
void CamController::getFaceBox(camera_fb_t *fb, FaceBox *result) {
  if (!fb || !result) {
    return;
  }

  result->detected = false;

  const std::vector<int> inputShape = {
    static_cast<int>(fb->height),
    static_cast<int>(fb->width),
    3
  };

  auto &candidates = this->faceDetectorStage1.infer(
    reinterpret_cast<uint16_t *>(fb->buf),
    inputShape
  );

  auto &detections = this->faceDetectorStage2.infer(
    reinterpret_cast<uint16_t *>(fb->buf),
    inputShape,
    candidates
  );

  if (!detections.empty()) {
    const auto &face = detections.front();
    if (face.box.size() >= 4) {
      result->x = face.box[0];
      result->y = face.box[1];
      const int x2 = face.box[2];
      const int y2 = face.box[3];
      result->width = x2 - result->x + 1;
      result->height = y2 - result->y + 1;
      result->detected = true;
    }
  }
}

bool CamController::captureJpeg(std::vector<uint8_t> &jpegBuffer) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("Camera capture failed");
    return false;
  }

  if (frame->format == PIXFORMAT_JPEG) {
    jpegBuffer.assign(frame->buf, frame->buf + frame->len);
    esp_camera_fb_return(frame);
    return true;
  }

  const bool success = this->encodeFrameToJpeg(frame, jpegBuffer);
  esp_camera_fb_return(frame);
  return success;
}

bool CamController::captureJpegWithFaceBox(std::vector<uint8_t> &jpegBuffer) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("Camera capture failed");
    return false;
  }

  std::vector<uint8_t> rgbBuffer;
  camera_fb_t rgbFrame = {};
  if (!this->decodeJpegToRgb565(frame, rgbBuffer, rgbFrame)) {
    esp_camera_fb_return(frame);
    return false;
  }

  FaceBox faceBox = {};
  this->getFaceBox(&rgbFrame, &faceBox);
  this->storeFaceBox(faceBox, rgbFrame.width, rgbFrame.height);
  if (faceBox.detected) {
    this->drawFaceBox(&rgbFrame, faceBox);
  }

  const bool success = this->encodeFrameToJpeg(&rgbFrame, jpegBuffer);
  esp_camera_fb_return(frame);
  return success;
}

bool CamController::encodeFrameToJpeg(camera_fb_t *frame, std::vector<uint8_t> &jpegBuffer) {
  if (!frame) {
    return false;
  }

  uint8_t *jpegData = nullptr;
  size_t jpegLength = 0;
  const bool converted = fmt2jpg(
    frame->buf,
    frame->len,
    frame->width,
    frame->height,
    frame->format,
    this->photoJpegQuality,
    &jpegData,
    &jpegLength
  );

  if (!converted || !jpegData || jpegLength == 0) {
    Serial.println("JPEG conversion failed");
    if (jpegData) {
      free(jpegData);
    }
    return false;
  }

  jpegBuffer.assign(jpegData, jpegData + jpegLength);
  free(jpegData);
  return true;
}

void CamController::applyUltraWideSensorPreset(sensor_t *sensor) {
  if (!sensor) {
    return;
  }

  sensor->set_framesize(sensor, this->currentFrameSize);
  // Recommended defaults for ultra-wide livestreaming (balanced FPS / quality)
  sensor->set_quality(sensor, this->photoJpegQuality);
  sensor->set_brightness(sensor, 1);
  sensor->set_contrast(sensor, 0);
  sensor->set_saturation(sensor, 0);

  sensor->set_exposure_ctrl(sensor, 1);
  sensor->set_aec2(sensor, 1);
  sensor->set_ae_level(sensor, 0);
  sensor->set_gain_ctrl(sensor, 1);
  sensor->set_agc_gain(sensor, 0);
  sensor->set_gainceiling(sensor, GAINCEILING_2X);

  sensor->set_whitebal(sensor, 1);
  sensor->set_awb_gain(sensor, 1);
  sensor->set_wb_mode(sensor, 0);

  sensor->set_lenc(sensor, 1);
  sensor->set_dcw(sensor, 1);
  sensor->set_bpc(sensor, 0);
  sensor->set_wpc(sensor, 1);
  sensor->set_raw_gma(sensor, 1);
  sensor->set_colorbar(sensor, 0);
  sensor->set_hmirror(sensor, 0);
  sensor->set_vflip(sensor, 0);
}

void CamController::setJpegQuality(uint8_t quality) {
  const uint8_t clamped = constrain(quality, 10u, 63u);
  this->photoJpegQuality = clamped;
  if (this->activeSensor) {
    this->activeSensor->set_quality(this->activeSensor, clamped);
  }
}

uint8_t CamController::getJpegQuality() const {
  return this->photoJpegQuality;
}

void CamController::setFrameSize(framesize_t frameSize) {
  this->currentFrameSize = frameSize;
  if (this->activeSensor) {
    this->activeSensor->set_framesize(this->activeSensor, frameSize);
  }
}

framesize_t CamController::getFrameSize() const {
  return this->currentFrameSize;
}

bool CamController::getLastFaceBox(FaceBox &box, uint16_t &width, uint16_t &height, uint64_t &timestamp) const {
  portENTER_CRITICAL(&const_cast<CamController*>(this)->faceMux);
  box = this->lastFaceBox;
  width = this->lastFaceFrameWidth;
  height = this->lastFaceFrameHeight;
  timestamp = this->lastFaceTimestamp;
  portEXIT_CRITICAL(&const_cast<CamController*>(this)->faceMux);
  return box.detected;
}

void CamController::storeFaceBox(const FaceBox &box, uint16_t width, uint16_t height) {
  portENTER_CRITICAL(&this->faceMux);
  this->lastFaceBox = box;
  this->lastFaceFrameWidth = width;
  this->lastFaceFrameHeight = height;
  this->lastFaceTimestamp = esp_timer_get_time();
  portEXIT_CRITICAL(&this->faceMux);
}

void CamController::drawFaceBox(camera_fb_t *frame, const FaceBox &faceBox) {
  if (!frame || frame->format != PIXFORMAT_RGB565) {
    return;
  }

  const int width = frame->width;
  const int height = frame->height;
  if (width <= 0 || height <= 0) {
    return;
  }

  const int left = constrain(faceBox.x, 0, width - 1);
  const int top = constrain(faceBox.y, 0, height - 1);
  const int right = constrain(faceBox.x + faceBox.width - 1, 0, width - 1);
  const int bottom = constrain(faceBox.y + faceBox.height - 1, 0, height - 1);
  if (left >= right || top >= bottom) {
    return;
  }

  uint16_t *pixels = reinterpret_cast<uint16_t *>(frame->buf);
  for (int thickness = 0; thickness < this->FACE_BOX_THICKNESS; ++thickness) {
    const int topRow = std::max(top - thickness, 0);
    const int bottomRow = std::min(bottom + thickness, height - 1);
    for (int x = left; x <= right; ++x) {
      pixels[topRow * width + x] = this->FACE_BOX_COLOR;
      pixels[bottomRow * width + x] = this->FACE_BOX_COLOR;
    }

    const int leftCol = std::max(left - thickness, 0);
    const int rightCol = std::min(right + thickness, width - 1);
    for (int y = top; y <= bottom; ++y) {
      pixels[y * width + leftCol] = this->FACE_BOX_COLOR;
      pixels[y * width + rightCol] = this->FACE_BOX_COLOR;
    }
  }
}

bool CamController::decodeJpegToRgb565(const camera_fb_t *src,
                                       std::vector<uint8_t> &rgbBuffer,
                                       camera_fb_t &rgbFrame) {
  if (!src) {
    return false;
  }

  if (src->format == PIXFORMAT_RGB565) {
    rgbFrame = *src;
    rgbFrame.format = PIXFORMAT_RGB565;
    return true;
  }

  uint16_t width = src->width;
  uint16_t height = src->height;
  if (width == 0 || height == 0) {
    resolveFrameSize(this->currentFrameSize, width, height);
  }

  rgbBuffer.resize(static_cast<size_t>(width) * height * 2);
  if (!jpg2rgb565(src->buf, src->len, rgbBuffer.data(), JPG_SCALE_NONE)) {
    rgbBuffer.clear();
    return false;
  }

  rgbFrame = *src;
  rgbFrame.buf = rgbBuffer.data();
  rgbFrame.len = rgbBuffer.size();
  rgbFrame.format = PIXFORMAT_RGB565;
  rgbFrame.width = width;
  rgbFrame.height = height;
  return true;
}

void CamController::resolveFrameSize(framesize_t frameSize, uint16_t &width, uint16_t &height) {
  switch (frameSize) {
    case FRAMESIZE_QQVGA: width = 160; height = 120; break;
    case FRAMESIZE_QVGA: width = 320; height = 240; break;
    case FRAMESIZE_CIF: width = 400; height = 296; break;
    case FRAMESIZE_VGA: width = 640; height = 480; break;
    case FRAMESIZE_SVGA: width = 800; height = 600; break;
    case FRAMESIZE_XGA: width = 1024; height = 768; break;
    case FRAMESIZE_SXGA: width = 1280; height = 1024; break;
    case FRAMESIZE_UXGA: width = 1600; height = 1200; break;
    default:
      width = 640;
      height = 480;
      break;
  }
}
