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

#endif /* Pins_Arduino_h */
