#ifndef CAM_H
#define CAM_H

#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>
#include "Arduino.h"

#include "../../include/config.h"

class Camera {
private:
    httpd_handle_t stream_httpd = NULL;
    static const char* _STREAM_CONTENT_TYPE;
    static const char* _STREAM_BOUNDARY;
    static const char* _STREAM_PART;

    // Handler-Funktion für Stream-Anfragen
    static esp_err_t streamHandler(httpd_req_t *req);

public:
    Camera();
    ~Camera();

    // Initialisierung der Kamera
    bool init();

    // Starten des Kamera-Webservers
    void startStreamServer();

    // Verbindung mit WLAN herstellen
    bool connectToWiFi();

    // Ausgabe der IP-Adresse
    void printStreamInfo();
};

#endif // CAM_H