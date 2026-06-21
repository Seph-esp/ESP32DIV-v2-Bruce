# ESP32 DIV-v2 Bruce Port

This repository provides support for the CiferTech ESP32 DIV-v2 within the Bruce firmware ecosystem. The codebase was generated using AI assistance. The firmware is currently in a near-functional state: the device initializes and receives RF signals, but transmission is non-functional.

# Features

* ESP32-S3 16MB support with updated pin bindings.
* Integration for three NRF24 modules with fallback support.
* Functional buzzer and NeoPixel effects.
* ESC button mapping assigned to the Boot button.
* UI refinements and touch calibration corrections.
* Comprehensive RF listening capabilities.

# Known Issues and Limitations

* NRF24 transmission: Output is either critically weak or non-existent.
* Sub-GHz transmission: Untested.
* System stability: Frequent softlocks and bugs occur during standard operation.
* Spectrum analysis: Sub-GHz RSSI and regular spectrum modes are unstable. Altering ranges or bands triggers device softlocks.
* UART mode: Listening to NRF24 via UART induces significant lag and partial softlocks.
* Infrared: RX is disabled (pin set to -1) to maintain NRF24 stability. 
* Performance: Navigating settings while the NRF24 jammer is active causes massive latency.

# Project Status and Disclaimer

This repository is no longer maintained. The code is provided as-is without future updates. Installation is performed at the user's risk. The author assumes no liability for bricked hardware or functional failure.

# Flashing Instructions

Use the following web flasher for installation:
https://cifertech.github.io/FirmwareHub/

If the device is not detected by the COM ports, install the Silicon Labs VCP driver:
https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers
