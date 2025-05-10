#include "light.hpp"

Light::Light() {
    strip = new Adafruit_NeoPixel(LIGHT_COUNT, LIGHT_PIN, NEO_GRBW + NEO_KHZ800);
}

Light::~Light() {
    delete strip;
}

void Light::init() {
    strip->begin();
    strip->setBrightness(brightness);
    strip->show();
}

void Light::setBrightness(uint8_t b) {
    brightness = b;
    strip->setBrightness(brightness);
    strip->show();
}

void Light::setColor(uint32_t color) {
    for (uint16_t i = 0; i < LIGHT_COUNT; ++i) {
        strip->setPixelColor(i, color);
    }
    strip->show();
}

void Light::setPixelColor(uint8_t pixel, uint32_t color) {
    if (pixel < LIGHT_COUNT) {
        strip->setPixelColor(pixel, color);
        strip->show();
    }
}

void Light::show() {
    strip->show();
}

void Light::clear() {
    strip->clear();
    strip->show();
}

void Light::loop() {
    if (this->counter >= 255) {
        this->counter = 0;
    }

    for (uint16_t i = 0; i < LIGHT_COUNT; ++i) {
        uint32_t color = strip->Color(this->counter, this->counter, this->counter);
        strip->setPixelColor(i, color);
    }
}