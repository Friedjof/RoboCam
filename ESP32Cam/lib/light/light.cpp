#include "light.hpp"


WS2812FX strip = WS2812FX(LIGHT_COUNT, LIGHT_PIN, NEO_GRB + NEO_KHZ800);


Light::Light() {
}

Light::~Light() {
}

void Light::init() {
    strip.init();
    strip.setBrightness(brightness);
    strip.setSpeed(LIGHT_SPEED);

    strip.setCustomMode(blink3_then_off);
    strip.setMode(FX_MODE_CUSTOM);

    strip.start();
}

void Light::setBrightness(uint8_t b) {
    brightness = b;
    strip.setBrightness(brightness);
    this->show();
}

void Light::setColor(uint32_t color) {
    for (uint16_t i = 0; i < LIGHT_COUNT; ++i) {
        strip.setPixelColor(i, color);
    }
    this->show();
}

void Light::setColor(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < LIGHT_COUNT; ++i) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    this->show();
}

void Light::setPixelColor(uint32_t pixel, uint32_t color) {
    strip.setPixelColor((pixel + LIGHT_START_POS) % LIGHT_COUNT, color);
    this->show();
}

void Light::show() {
    strip.show();
}

uint16_t Light::blink3_then_off(void) {
    static uint8_t blink_count = 0;
    static bool led_on = false;
    static unsigned long last_change = 0;
    const unsigned long interval = 500;
  
    unsigned long now = millis();
  
    if (blink_count < 3) {
        if (now - last_change >= interval) {
            last_change = now;
            led_on = !led_on;
            for (uint16_t i = 0; i < LIGHT_COUNT; i++) {
                strip.setPixelColor(i, led_on ? strip.Color(255, 255, 255) : 0);
            }
            strip.show();
            if (!led_on) blink_count++;
        }
    } else {
        for (uint16_t i = 0; i < LIGHT_COUNT; i++) {
            strip.setPixelColor(i, 0);
        }
        strip.show();
    }

    return 10;
}


void Light::clear() {
    strip.clear();
    this->show();
}

bool Light::updateMode() {
    bool result = millis() - this->lastCall > LIGHT_SPEED;

    this->lastCall = result ? millis() : this->lastCall;

    return result;
}

void Light::loop() {
    strip.service();
}