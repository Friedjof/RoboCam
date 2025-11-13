#ifndef CAM_CONTROLLER_HPP
#define CAM_CONTROLLER_HPP

#include <Arduino.h>
#include <esp_log.h>
#include <list>
#include <vector>

#include "esp_camera.h"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"

#include "ServoService.hpp"

#include "CONFIG.hpp"


struct FaceBox {
    int x;         // X-Koordinate (links oben)
    int y;         // Y-Koordinate (links oben)
    int width;     // Breite des Vierecks
    int height;    // Höhe des Vierecks
    bool detected; // Wurde ein Gesicht erkannt?
};


class CamController {
public:
  CamController();

  void begin(ServoService* servoX, ServoService* servoY);
  void run();

  void getFaceBox(camera_fb_t *fb, FaceBox *result);
  bool captureRotatedJpeg(std::vector<uint8_t> &jpegBuffer);

private:
  ServoService* servoX;
  ServoService* servoY;

  uint64_t lastFaceDetectTime = 0;

  static constexpr int MAX_SERVO_STEP = 5;
  static constexpr int STEP_SCALE = 60;
  static constexpr uint8_t PHOTO_JPEG_QUALITY = 80;

  HumanFaceDetectMSR01 faceDetectorStage1;
  HumanFaceDetectMNP01 faceDetectorStage2;

  bool rotateRgb565Frame90CW(const camera_fb_t *fb,
                             std::vector<uint8_t> &rotatedBuffer,
                             uint16_t &rotatedWidth,
                             uint16_t &rotatedHeight);
};

#endif // CAM_CONTROLLER_HPP
