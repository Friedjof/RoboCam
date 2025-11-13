#include "CamController.hpp"
#include "img_converters.h"


CamController::CamController()
  : servoX(nullptr),
    servoY(nullptr),
    faceDetectorStage1(0.1F, 0.5F, 10, 0.2F),
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
  cameraConfig.pixel_format = PIXFORMAT_RGB565;
  cameraConfig.xclk_freq_hz = 20000000;
  cameraConfig.frame_size = FRAMESIZE_240X240;
  cameraConfig.jpeg_quality = 12;
  cameraConfig.fb_count = 2;
  cameraConfig.fb_location = CAMERA_FB_IN_PSRAM;
  cameraConfig.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }

  Serial.printf("Camera ready. Frame size: %d, quality: %d\n", cameraConfig.frame_size, cameraConfig.jpeg_quality);
}

void CamController::run() {
  if (millis() - this->lastFaceDetectTime >= FACE_DETECT_INTERVAL) {
    this->lastFaceDetectTime = millis();

    camera_fb_t *rawFrame = esp_camera_fb_get();
    if (!rawFrame) {
      Serial.println("Camera capture failed");
      return;
    }
    std::vector<uint8_t> rotatedBuffer;
    uint16_t rotatedWidth = 0;
    uint16_t rotatedHeight = 0;
    camera_fb_t workingFrame = *rawFrame;

    if (this->rotateRgb565Frame90CW(rawFrame, rotatedBuffer, rotatedWidth, rotatedHeight)) {
      workingFrame.width = rotatedWidth;
      workingFrame.height = rotatedHeight;
      workingFrame.len = rotatedBuffer.size();
      workingFrame.buf = rotatedBuffer.data();
    }

    const int frameWidth = workingFrame.width;
    const int frameHeight = workingFrame.height;

    FaceBox faceBox = {};

    // Gesichtserkennung auf der rotierten Darstellung durchführen
    this->getFaceBox(&workingFrame, &faceBox);
    esp_camera_fb_return(rawFrame);

    if (faceBox.detected) {
      Serial.printf("Face detected at x:%d y:%d w:%d h:%d\n", faceBox.x, faceBox.y, faceBox.width, faceBox.height);
      if (frameWidth > 0 && frameHeight > 0) {
        const int faceCenterX = faceBox.x + faceBox.width / 2;
        const int faceCenterY = faceBox.y + faceBox.height / 2;
        const int frameCenterX = frameWidth / 2;
        const int frameCenterY = frameHeight / 2;
        if (this->servoX) {
          const int errorX = faceCenterX - frameCenterX;
          int stepX = constrain(errorX * this->STEP_SCALE / frameWidth, -this->MAX_SERVO_STEP, this->MAX_SERVO_STEP);
          if (stepX != 0) {
            const int currentX = this->servoX->getAngle();
            this->servoX->setPosition(
              constrain(currentX - stepX, ServoService::MIN_ANGLE, ServoService::MAX_ANGLE)
            );
          }
        }
        if (this->servoY) {
          const int errorY = faceCenterY - frameCenterY;
          int stepY = constrain(errorY * this->STEP_SCALE / frameHeight, -this->MAX_SERVO_STEP, this->MAX_SERVO_STEP);
          if (stepY != 0) {
            const int currentY = this->servoY->getAngle();
            this->servoY->setPosition(
              constrain(currentY - stepY, ServoService::MIN_ANGLE, ServoService::MAX_ANGLE)
            );
          }
        }
      }
    } else {
      Serial.println("No face detected");
    }
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

bool CamController::captureRotatedJpeg(std::vector<uint8_t> &jpegBuffer) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("Camera capture failed");
    return false;
  }

  std::vector<uint8_t> rotatedBuffer;
  uint16_t rotatedWidth = 0;
  uint16_t rotatedHeight = 0;
  const bool rotated = this->rotateRgb565Frame90CW(frame, rotatedBuffer, rotatedWidth, rotatedHeight);
  esp_camera_fb_return(frame);

  if (!rotated) {
    Serial.println("Frame rotation failed");
    return false;
  }

  uint8_t *jpegData = nullptr;
  size_t jpegLength = 0;
  const bool converted = fmt2jpg(
    rotatedBuffer.data(),
    rotatedBuffer.size(),
    rotatedWidth,
    rotatedHeight,
    PIXFORMAT_RGB565,
    this->PHOTO_JPEG_QUALITY,
    &jpegData,
    &jpegLength
  );

  if (!converted || !jpegData || jpegLength == 0) {
    Serial.println("JPEG conversion after rotation failed");
    if (jpegData) {
      free(jpegData);
    }
    return false;
  }

  jpegBuffer.assign(jpegData, jpegData + jpegLength);
  free(jpegData);
  return true;
}

bool CamController::rotateRgb565Frame90CW(const camera_fb_t *frame,
                                          std::vector<uint8_t> &rotatedBuffer,
                                          uint16_t &rotatedWidth,
                                          uint16_t &rotatedHeight) {
  if (!frame) {
    return false;
  }

  if (frame->format != PIXFORMAT_RGB565) {
    Serial.println("Rotation only implemented for RGB565 frames");
    return false;
  }

  const uint16_t srcWidth = frame->width;
  const uint16_t srcHeight = frame->height;
  rotatedWidth = srcHeight;
  rotatedHeight = srcWidth;

  rotatedBuffer.resize(static_cast<size_t>(rotatedWidth) * rotatedHeight * 2);

  const uint16_t *srcPixels = reinterpret_cast<const uint16_t *>(frame->buf);
  uint16_t *dstPixels = reinterpret_cast<uint16_t *>(rotatedBuffer.data());

  for (int y = 0; y < srcHeight; ++y) {
    for (int x = 0; x < srcWidth; ++x) {
      const uint16_t pixel = srcPixels[y * srcWidth + x];
      const int newX = rotatedWidth - 1 - y;
      const int newY = x;
      dstPixels[newY * rotatedWidth + newX] = pixel;
    }
  }

  return true;
}
