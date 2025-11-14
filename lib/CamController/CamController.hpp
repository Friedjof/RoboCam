#ifndef CAM_CONTROLLER_HPP
#define CAM_CONTROLLER_HPP

#include <Arduino.h>
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
  void runAutoTracking();
  void setJpegQuality(uint8_t quality);
  uint8_t getJpegQuality() const;
  void setFrameSize(framesize_t frameSize);
  framesize_t getFrameSize() const;
  static void resolveFrameSize(framesize_t frameSize, uint16_t &width, uint16_t &height);
  bool getLastFaceBox(FaceBox &box, uint16_t &width, uint16_t &height, uint64_t &timestamp) const;

  void getFaceBox(camera_fb_t *fb, FaceBox *result);
  bool captureJpeg(std::vector<uint8_t> &jpegBuffer);
  bool captureJpegWithFaceBox(std::vector<uint8_t> &jpegBuffer);

private:
  ServoService* servoX = nullptr;
  ServoService* servoY = nullptr;
  uint64_t lastFaceDetectTime = 0;
  uint8_t photoJpegQuality = 12;
  framesize_t currentFrameSize = FRAMESIZE_VGA;
  FaceBox lastFaceBox = {};
  uint16_t lastFaceFrameWidth = 0;
  uint16_t lastFaceFrameHeight = 0;
  uint64_t lastFaceTimestamp = 0;
  portMUX_TYPE faceMux = portMUX_INITIALIZER_UNLOCKED;

  static constexpr int MAX_SERVO_STEP = 3;
  static constexpr int STEP_SCALE = 60;
  static constexpr uint16_t FACE_BOX_COLOR = 0xF800; // RGB565 red
  static constexpr uint8_t FACE_BOX_THICKNESS = 2;

  HumanFaceDetectMSR01 faceDetectorStage1;
  HumanFaceDetectMNP01 faceDetectorStage2;
  sensor_t* activeSensor = nullptr;

  void applyUltraWideSensorPreset(sensor_t *sensor);
  bool encodeFrameToJpeg(camera_fb_t *frame, std::vector<uint8_t> &jpegBuffer);
  void drawFaceBox(camera_fb_t *frame, const FaceBox &faceBox);
  bool decodeJpegToRgb565(const camera_fb_t *src, std::vector<uint8_t> &rgbBuffer, camera_fb_t &rgbFrame);
  void storeFaceBox(const FaceBox &box, uint16_t width, uint16_t height);
};

#endif // CAM_CONTROLLER_HPP
