#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>
#include <WS2812FX.h>
#include "../../include/config.h"

extern WS2812FX strip;

class Light {
private:
    uint8_t brightness = LIGHT_BRIGHTNESS;
    uint32_t counter = 0;
    uint64_t lastCall = 0;

    bool updateMode();

public:
    Light();
    ~Light();

    void init();

    static uint16_t blink3_then_off(void);

    void setBrightness(uint8_t brightness);
    void setColor(uint32_t color);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint32_t pixel, uint32_t color);

    void show();
    void clear();
    void loop();
};

#endif // LIGHT_H