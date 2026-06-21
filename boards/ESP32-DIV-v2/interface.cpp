#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <LittleFS.h>
#include <PCF8574.h>
#include <interface.h>

// PCF8574 instance (usually I2C address is 0x20 on ESP32-DIV V2)
PCF8574 pcf(0x20);
static bool pcfInitialized = false;

// Button pins (swapped Left/Right and Up/Down for correct layout)
#define BTN_UP     3
#define BTN_DOWN   4
#define BTN_LEFT   7
#define BTN_RIGHT  5
#define BTN_SELECT 6

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    Serial.println("[ESP32-DIV-v2] _setup_gpio starting");
    
    pinMode(0, INPUT_PULLUP); // BOOT Button

    // Initialize I2C for PCF8574
    Wire.begin(GROVE_SDA, GROVE_SCL);
    
    // Initialize PCF8574 pins
    pcf.pinMode(BTN_UP, INPUT_PULLUP);
    pcf.pinMode(BTN_DOWN, INPUT_PULLUP);
    pcf.pinMode(BTN_LEFT, INPUT_PULLUP);
    pcf.pinMode(BTN_RIGHT, INPUT_PULLUP);
    pcf.pinMode(BTN_SELECT, INPUT_PULLUP);
    
    // Try to begin PCF8574 at address 0x20
    if (pcf.begin()) {
        pcfInitialized = true;
        Serial.println("[ESP32-DIV-v2] PCF8574 initialized successfully at 0x20");
    } else {
        Serial.println("[ESP32-DIV-v2] WARNING: PCF8574 not found at 0x20");
    }
    
    // Touch screen CS
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);

    // Drive dedicated SPI CS pins HIGH to prevent bus contention on boot
    pinMode(4, OUTPUT); // NRF24 CS #1
    digitalWrite(4, HIGH);
    pinMode(5, OUTPUT); // CC1101 CS
    digitalWrite(5, HIGH);
    pinMode(10, OUTPUT); // SD CS
    digitalWrite(10, HIGH);
    pinMode(48, INPUT_PULLUP); // NRF24 CS #2
    pinMode(21, INPUT_PULLUP); // NRF24 CS #3

    bruceConfigPins.irRx = RXLED;
    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // Touchscreen calibration load
    uint16_t calData[5];
    File caldata = LittleFS.open("/calData", "r");

    if (!caldata) {
        tft.setRotation(ROTATION);
        tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 10);

        caldata = LittleFS.open("/calData", "w");
        if (caldata) {
            caldata.printf(
                "%d\n%d\n%d\n%d\n%d\n", calData[0], calData[1], calData[2], calData[3], calData[4]
            );
            caldata.close();
        }
    } else {
        Serial.print("\ntft Calibration data: ");
        for (int i = 0; i < 5; i++) {
            String line = caldata.readStringUntil('\n');
            calData[i] = line.toInt();
            Serial.printf("%d, ", calData[i]);
        }
        Serial.println();
        caldata.close();
    }
    tft.setTouch(calData);
    tft.setRotation(bruceConfigPins.rotation);

    // Brightness control setup
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
}

void recalibrateTouch() {
    uint16_t calData[5];
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(ROTATION);
    tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 10);

    File caldata = LittleFS.open("/calData", "w");
    if (caldata) {
        caldata.printf(
            "%d\n%d\n%d\n%d\n%d\n", calData[0], calData[1], calData[2], calData[3], calData[4]
        );
        caldata.close();
    }
    tft.setTouch(calData);
    tft.setRotation(bruceConfigPins.rotation);
}

/*********************************************************************
** Function: setBrightness
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        // Read Touchscreen
        TouchPoint t;
        checkPowerSaveTime();
        bool _IH_touched = tft.getTouch(&t.x, &t.y);
        if (_IH_touched) {
            NextPress = false;
            PrevPress = false;
            UpPress = false;
            DownPress = false;
            SelPress = false;
            EscPress = false;
            AnyKeyPress = false;
            NextPagePress = false;
            PrevPagePress = false;
            touchPoint.pressed = false;
            _IH_touched = false;

            if (!wakeUpScreen()) AnyKeyPress = true;
            else { yield(); return; }

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
            d_tmp = millis();
            return;
        }

        // Read BOOT Button and PCF8574 Buttons
        bool boot_pressed = (digitalRead(0) == LOW);
        bool pcf_up = false, pcf_down = false, pcf_left = false, pcf_right = false, pcf_select = false;
        
        if (pcfInitialized) {
            pcf_up = (pcf.digitalRead(BTN_UP) == LOW);
            pcf_down = (pcf.digitalRead(BTN_DOWN) == LOW);
            pcf_left = (pcf.digitalRead(BTN_LEFT) == LOW);
            pcf_right = (pcf.digitalRead(BTN_RIGHT) == LOW);
            pcf_select = (pcf.digitalRead(BTN_SELECT) == LOW);
        }

        bool anyPressed = pcf_up || pcf_down || pcf_left || pcf_right || pcf_select || boot_pressed;
        if (anyPressed) {
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
                
                // Map buttons to global navigation state
                UpPress = pcf_up;
                PrevPress = pcf_up;
                DownPress = pcf_down;
                NextPress = pcf_down;
                EscPress = pcf_left || boot_pressed;
                SelPress = pcf_select;
            }
            d_tmp = millis();
        }
    END:
        yield();
    }
}

/*********************************************************************
** Function: powerOff
** Turns off the device
**********************************************************************/
void powerOff() {
    // Turn off backlight
    ledcWrite(TFT_BL, 0);
    // Send display to sleep
    tft.writecommand(0x10); // SLPIN
    // Wake on BOOT button (GPIO0)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: goToDeepSleep
**********************************************************************/
void goToDeepSleep() { powerOff(); }

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {
    // If BOOT button is held, power off
    int c = 0;
    while (digitalRead(GPIO_NUM_0) == LOW) {
        delay(100);
        c++;
        if (c > 20) { // 2 seconds
            powerOff();
        }
    }
}

/*********************************************************************
** Function: isCharging
**********************************************************************/
bool isCharging() { return false; }
