#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// USB Serial / USB CDC on boot
static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

static const uint8_t SS = 5;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;

// Deepsleep wakeup (BOOT button)
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT LOW

// NeoPixel LED configurations
#define HAS_RGB_LED 1
#define LED_COUNT 4
#define RGB_LED 1
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COLOR_STEP 15

#endif /* Pins_Arduino_h */
