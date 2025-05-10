#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "../../include/config.h"

class Light {
private:
    Adafruit_NeoPixel* strip = nullptr;
    uint8_t brightness = LIGHT_BRIGHTNESS;
    uint16_t counter = 0;

public:
    Light();
    ~Light();

    void init();
    void setBrightness(uint8_t brightness);
    void setColor(uint32_t color);
    void setPixelColor(uint8_t pixel, uint32_t color);
    void show();
    void clear();
    void loop();
};