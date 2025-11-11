#ifndef CAM_CONTROLLER_HPP
#define CAM_CONTROLLER_HPP

#include <Arduino.h>
#include <esp_log.h>

#include "esp_camera.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"


#include "ServoService.hpp"

#include "CONFIG.hpp"


struct FaceBox {
    int x;        // X-Koordinate (links oben)
    int y;        // Y-Koordinate (links oben)
    int width;    // Breite des Vierecks
    int height;   // Höhe des Vierecks
    bool detected; // Wurde ein Gesicht erkannt?
};


class CamController {
public:
  CamController();

  void begin(ServoService* servoX, ServoService* servoY);
  void run();

  FaceBox getFaceBox(camera_fb_t *fb);

private:
  ServoService* servoX;
  ServoService* servoY;

  uint64_t lastFaceDetectTime = 0;

  mtmn_config_t mtmn_config = {
    .min_face = 80,
    .pyramid = 0.709f,
    .pyramid_times = 4,
    .p_threshold = {0.6f, 0.7f, 4},
    .r_threshold = {0.7f, 0.7f, 4},
    .o_threshold = {0.7f, 0.7f, 4},
    .type = FAST,
  };

  camera_fb_t* fb = nullptr;
};

#endif // CAM_CONTROLLER_HPP
