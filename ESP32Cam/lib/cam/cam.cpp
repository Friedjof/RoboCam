#include "cam.hpp"
#include "img_converters.h"
#include "esp_timer.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* Camera::_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
const char* Camera::_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
const char* Camera::_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

Camera::Camera() {
    // Konstruktor
}

Camera::~Camera() {
    // Destruktor
}

bool Camera::init() {
    // Brownout-Detektor deaktivieren
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG; 
    
    if(psramFound()){
        config.frame_size = FRAMESIZE_SVGA;  // Kleinere Auflösung für WebSocket-Streaming
        config.jpeg_quality = 12;            // Höhere Qualität = größere Zahl = kleinere Datei
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_CIF;
        config.jpeg_quality = 10;
        config.fb_count = 1;
    }
    
    // Kamera initialisieren
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }
    
    return true;
}

bool Camera::connectToWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // 15 Sekunden auf Verbindung warten
    int timeout = 30; // 30 * 500ms = 15s
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        Serial.print(".");
        timeout--;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi");
        return false;
    }
    
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
}

void Camera::printStreamInfo() {
    Serial.println("");
    Serial.print("Camera Stream Ready! Go to: http://");
    Serial.println(WiFi.localIP());
}

camera_fb_t* Camera::getFrame() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("esp_camera_fb_get() failed!");
    }
    return fb;
}

void Camera::returnFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}
