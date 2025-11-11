#include "CamController.hpp"

CamController::CamController() : servoX(nullptr), servoY(nullptr) {}

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
  cameraConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  cameraConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
  cameraConfig.pin_reset = RESET_GPIO_NUM;
  cameraConfig.pixel_format = PIXFORMAT_JPEG;
  cameraConfig.xclk_freq_hz = 20000000;
  if (psramFound()) {
    cameraConfig.frame_size = FRAMESIZE_UXGA;
    cameraConfig.jpeg_quality = 10;
    cameraConfig.fb_count = 2;
  } else {
    cameraConfig.frame_size = FRAMESIZE_SVGA;
    cameraConfig.jpeg_quality = 12;
    cameraConfig.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed: 0x%x", err);
    return;
  }

  ESP_LOGI(TAG, "Camera ready. Frame size: %d, quality: %d", cameraConfig.frame_size, cameraConfig.jpeg_quality);
}

void CamController::run() {
  if (millis() - this->lastFaceDetectTime >= FACE_DETECT_INTERVAL) {
    this->lastFaceDetectTime = millis();

    // Frame holen
    fb = esp_camera_fb_get();
    if (!fb) {
      ESP_LOGE(TAG, "Camera capture failed");
      return;
    }

    // Gesichtserkennung durchführen
    FaceBox faceBox = getFaceBox(fb);

    if (faceBox.detected) {
      ESP_LOGI(TAG, "Face detected at x:%d y:%d w:%d h:%d", faceBox.x, faceBox.y, faceBox.width, faceBox.height);
      // Hier könnte man die Servo-Positionen basierend auf der Gesichtslage anpassen
    } else {
      ESP_LOGI(TAG, "No face detected");
    }
  }
}

// Funktion gibt die Position des ersten erkannten Gesichts zurück
FaceBox CamController::getFaceBox(camera_fb_t *fb) {
    FaceBox result = {0, 0, 0, 0, false};
    
    if (!fb) {
        return result;
    }
    
    // Bild in RGB888 Matrix konvertieren für Face Detection
    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix) {
        return result;
    }
    
    fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
    
    // Face Detection durchführen
    box_array_t *boxes = face_detect(image_matrix, &this->mtmn_config);
    
    // Wenn mindestens ein Gesicht erkannt wurde
    if (boxes != NULL && boxes->len > 0) {
        // Erstes erkanntes Gesicht
        box_t *face = &(boxes->box[0]);
        
        // Koordinaten extrahieren
        result.x = (int)face->box_p[0];  // X (links)
        result.y = (int)face->box_p[1];  // Y (oben)
        int x2 = (int)face->box_p[2];    // X (rechts)
        int y2 = (int)face->box_p[3];    // Y (unten)
        
        result.width = x2 - result.x + 1;
        result.height = y2 - result.y + 1;
        result.detected = true;
        
        // Cleanup
        free(boxes->box);
        free(boxes);
    }
    
    dl_matrix3du_free(image_matrix);
    return result;
}
