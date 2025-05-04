#ifndef CAM_H
#define CAM_H

#include <Arduino.h>
#include <WiFi.h>

#include "esp_camera.h"

#include "../../include/config.h"

class Camera {
private:
    // HTTP Stream-Formatierungen für MJPEG
    static const char* _STREAM_CONTENT_TYPE;
    static const char* _STREAM_BOUNDARY;
    static const char* _STREAM_PART;

public:
    Camera();
    ~Camera();

    // Initialisierung der Kamera
    bool init();

    // Verbindung mit WLAN herstellen
    bool connectToWiFi();

    // Ausgabe der IP-Adresse
    void printStreamInfo();
    
    // Kamera-Frame abrufen
    camera_fb_t* getFrame();
    
    // Frame zurückgeben, wenn nicht mehr benötigt
    void returnFrame(camera_fb_t* fb);
    
    // Helper-Methoden für Stream-Handling
    static const char* getStreamContentType() { return _STREAM_CONTENT_TYPE; }
    static const char* getStreamBoundary() { return _STREAM_BOUNDARY; }
    static const char* getStreamPart() { return _STREAM_PART; }
};

#endif // CAM_H