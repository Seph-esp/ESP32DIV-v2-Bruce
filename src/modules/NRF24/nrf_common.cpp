#include "nrf_common.h"
#include "../../core/mykeyboard.h"

RF24 NRFradio(bruceConfigPins.NRF24_bus.io0, bruceConfigPins.NRF24_bus.cs);
RF24 NRFradio2(47, 48); // Module 2: CE=47, CS=48
RF24 NRFradio3(14, 21); // Module 3: CE=14, CS=21

bool nrf1_online = false;
bool nrf2_online = false;
bool nrf3_online = false;

HardwareSerial NRFSerial = HardwareSerial(2); // Uses UART2 for External NRF's
SPIClass *NRFSPI;

void nrf_info() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("_Disclaimer_", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(15, 33);
    padprintln("These functions were made to be used in a controlled environment for STUDY only.");
    padprintln("");
    padprintln("DO NOT use these functions to harm people or companies, you can go to jail!");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");
    padprintln(
        "This device is VERY sensible to noise, so long wires or passing near VCC line can make "
        "things go wrong."
    );
    delay(1000);
    while (!check(AnyKeyPress));
}

bool nrf_start(NRF24_MODE mode) {
    bool result = false;
    if (mode == NRF_MODE_DISABLED) return false;

    if (CHECK_NRF_UART(mode)) {
        if (USBserial.getSerialOutput() == &Serial1) {
            displayError("(E) UART already in use", true);
            return false;
        }
        if (bruceConfigPins.uart_bus.rx < 0 || bruceConfigPins.uart_bus.tx < 0) {
            displayError("UART not configured", true);
            return false;
        }
        NRFSerial.begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
        Serial.println("NRF24 on Serial Started");
        result = true;
    };

    if (!CHECK_NRF_SPI(mode)) return result;

    // 1. Setup CS/CE pin modes and defaults
    // Primary NRF24 (Module 1)
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    pinMode(bruceConfigPins.NRF24_bus.io0, OUTPUT);
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);

#if defined(ESP32_DIV_V2)
    // Module 2
    pinMode(48, OUTPUT);
    digitalWrite(48, HIGH);
    pinMode(47, OUTPUT);
    digitalWrite(47, LOW);

    // Module 3
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    pinMode(14, OUTPUT);
    digitalWrite(14, LOW);
#endif

    // 2. Select SPI Class
    if (bruceConfigPins.NRF24_bus.mosi == (gpio_num_t)TFT_MOSI &&
        bruceConfigPins.NRF24_bus.mosi != GPIO_NUM_NC) { // (T_EMBED), CORE2 and others
#if TFT_MOSI > 0 // condition for Headless and 8bit displays (no SPI bus)
        NRFSPI = &tft.getSPIinstance();
#else
        NRFSPI = &SPI;
#endif

    } else if (bruceConfigPins.NRF24_bus.mosi == bruceConfigPins.SDCARD_bus.mosi) {
        // CC1101 shares SPI with SDCard (Cardputer and CYDs)
        NRFSPI = &sdcardSPI;
    } else if (bruceConfigPins.NRF24_bus.mosi == bruceConfigPins.CC1101_bus.mosi &&
               bruceConfigPins.NRF24_bus.mosi != bruceConfigPins.SDCARD_bus.mosi) {
        // Smoochie board shares CC1101 and NRF24 SPI bus
        NRFSPI = &CC_NRF_SPI;
    } else {
        NRFSPI = &SPI;
    }

    // Initialize SPI bus with correct NRF24 CS pin
    NRFSPI->begin(
        (int8_t)bruceConfigPins.NRF24_bus.sck,
        (int8_t)bruceConfigPins.NRF24_bus.miso,
        (int8_t)bruceConfigPins.NRF24_bus.mosi,
        (int8_t)bruceConfigPins.NRF24_bus.cs
    );
    delay(10);

    // 3. Begin RF24 chips
    nrf1_online = NRFradio.begin(
        NRFSPI,
        rf24_gpio_pin_t(bruceConfigPins.NRF24_bus.io0),
        rf24_gpio_pin_t(bruceConfigPins.NRF24_bus.cs)
    );

#if defined(ESP32_DIV_V2)
    nrf2_online = NRFradio2.begin(NRFSPI, 47, 48);
    nrf3_online = NRFradio3.begin(NRFSPI, 14, 21);

    // If primary NRF1 is not found, fallback NRFradio to use NRF2 or NRF3 pins
    if (!nrf1_online) {
        if (nrf2_online) {
            Serial.println("[NRF] Falling back primary NRFradio to Module 2 pins");
            NRFradio.begin(NRFSPI, 47, 48);
            nrf1_online = true; // Mark as online so standard tools work!
        } else if (nrf3_online) {
            Serial.println("[NRF] Falling back primary NRFradio to Module 3 pins");
            NRFradio.begin(NRFSPI, 14, 21);
            nrf1_online = true; // Mark as online so standard tools work!
        }
    }
#else
    nrf2_online = false;
    nrf3_online = false;
#endif

    Serial.printf("[NRF] Online modules: NRF1=%d, NRF2=%d, NRF3=%d\n", nrf1_online, nrf2_online, nrf3_online);

    if (nrf1_online || nrf2_online || nrf3_online) {
        result = true;
    } else {
        result = false;
    }

    return result;
}

NRF24_MODE nrf_setMode() {
    NRF24_MODE mode = NRF_MODE_DISABLED;
    options = {
        {"SPI Mode",  [&]() { mode = NRF_MODE_SPI; } },
        {"SPI UART",  [&]() { mode = NRF_MODE_UART; }},
        {"SPI BOTH",  [&]() { mode = NRF_MODE_BOTH; }},
        {"Main Menu", [=]() { returnToMenu = true; } }
    };
    loopOptions(options);
    return mode;
}
