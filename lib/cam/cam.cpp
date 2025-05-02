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
    if (stream_httpd) {
        httpd_stop(stream_httpd);
    }
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
        config.frame_size = FRAMESIZE_XGA;  // Auf VGA (640x480) reduzieren statt XGA
        config.jpeg_quality = 12;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_CIF;  // Noch kleiner: CIF (400x296)
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
    return true;
}

void Camera::startStreamServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = streamHandler,
        .user_ctx  = NULL
    };
    
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        Serial.println("Stream server started");
    } else {
        Serial.println("Failed to start stream server");
    }
}

void Camera::printStreamInfo() {
    Serial.println("");
    Serial.print("Camera Stream Ready! Go to: http://");
    Serial.println(WiFi.localIP());
}

esp_err_t Camera::streamHandler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  char part_buf[64];

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    // Verwende direkt das JPEG vom Framebuffer, keine Konvertierung
    size_t jpg_buf_len = fb->len;
    uint8_t *jpg_buf = fb->buf;

    // HTTP Header für Chunk schreiben
    size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, jpg_buf_len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res != ESP_OK) break;

    // Bilddaten senden
    res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_buf_len);
    if (res != ESP_OK) break;

    // Boundary senden
    res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if (res != ESP_OK) break;

    esp_camera_fb_return(fb); // Buffer zurückgeben
    fb = NULL;
  }

  if (fb) esp_camera_fb_return(fb);
  return res;
}
